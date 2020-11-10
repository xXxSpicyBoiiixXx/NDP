use std::io::Cursor;
use std::net::SocketAddr;
use std::marker::PhantomData;

use snafu::{ResultExt, Snafu, OptionExt, IntoError, ensure};
use async_trait::async_trait;
use tokio::io::{AsyncWriteExt, AsyncReadExt};
use tokio_util::codec::Decoder;
use bytes::BytesMut;
use rmps::{Serializer};
use serde::{Deserialize, Serialize};

use crate::buffer::{BufferContext, BufferState};
use crate::{message_kind, messages};
use std::borrow::{BorrowMut, Borrow};
use crate::messages::{ResponseBody, OkWithHandleResponse, OkWithBufferResponse, ErrResponse, MessageKindTagged, RequestBody, AllocRequest};

pub struct MessageCodec<'a, M: MessageDecoder> {
    header: &'a MessageHeaderDecoded,
    decoder: PhantomData<M>,
}

impl<M: MessageDecoder> MessageCodec<'_, M> {
    pub fn new(header: &MessageHeaderDecoded) -> MessageCodec<'_, M> {
        MessageCodec { header, decoder: Default::default() }
    }
}

impl<'a, M: MessageDecoder> Decoder for MessageCodec<'a, M>
    where <M as MessageDecoder>::Error: std::convert::From<std::io::Error>
{
    type Item = M::Message;
    type Error = M::Error;

    fn decode(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        M::read_from(src, self.header).map(Some)
    }
}


#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct MessageHeader {
    pub kind: String,
    pub length: u32,
}

#[derive(PartialEq)]
pub struct MessageHeaderDecoded {
    pub header: MessageHeader,
    pub header_len: u32,
}

impl MessageHeaderDecoded {
    pub fn data_len(&self) -> u32 {
        self.header.length
    }
    pub fn buf_target(&self) -> u32 {
        self.data_len()
    }
}

pub trait MessageDecoder {
    type Message;
    type Error: std::error::Error + 'static;

    fn read_from(buf: &mut BytesMut, meta: &MessageHeaderDecoded) -> Result<Self::Message, Self::Error>;
}

impl MessageDecoder for RequestBody {
    type Message = Self;
    type Error = rmps::decode::Error;
    fn read_from(buf: &mut BytesMut, meta: &MessageHeaderDecoded) -> Result<Self::Message, Self::Error> {
        let mut reader = Cursor::new(buf.clone());
        let result = match meta.header.kind.as_str() {
            message_kind::REQUEST_ALLOC => {
                let x = rmps::from_read::<_, messages::AllocRequest>(&mut reader)?;
                Ok(RequestBody::Alloc(x))
            },
            message_kind::REQUEST_READ => {
                let x = rmps::from_read::<_, messages::ReadRequest>(&mut reader)?;
                Ok(RequestBody::Read(x))
            },
            message_kind::REQUEST_WRITE => {
                let x = rmps::from_read::<_, messages::WriteRequest>(&mut reader)?;
                Ok(RequestBody::Write(x))
            },
            _ => Err(rmps::decode::Error::OutOfRange),
        };

        if let Ok(_) = result {
            buf.split_to(reader.borrow().position() as usize);
        }

        result
    }
}

impl<'a> MessageDecoder for ResponseBody {
    type Message = Self;
    type Error = rmps::decode::Error;

    fn read_from(buf: &mut BytesMut, meta: &MessageHeaderDecoded) -> Result<Self::Message, Self::Error> {
        let mut reader = Cursor::new(buf.clone());
        let kind = meta.header.kind.as_str();
        let result = match kind {
            message_kind::RESPONSE_OK_HANDLE => {
                let x = rmps::from_read::<_, OkWithHandleResponse>(&mut reader)?;
                Ok(ResponseBody::OkWithHandle(x))
            },
            message_kind::RESPONSE_OK_BUFFER => {
                let x = rmps::from_read::<_, OkWithBufferResponse>(&mut reader)?;
                Ok(ResponseBody::OkWithBuffer(x))
            },
            message_kind::RESPONSE_ERR => {
                let x = rmps::from_read::<_, ErrResponse>(&mut reader)?;
                Ok(ResponseBody::Err(x))
            },
            _ => {
                eprintln!("unsupported kind: '{}'", kind);
                Err(rmps::decode::Error::OutOfRange)
            },
        };

        if let Ok(_) = result {
            let len = reader.borrow().position() as usize;
            // println!("[read] chop {} bytes", len);
            buf.split_to(len);
        }

        result
    }
}


#[async_trait]
pub trait MessageEncoder {
    type Message: MessageKindTagged + Serialize;
    async fn write_to<'a, S: AsyncWriteExt + Unpin + Send>(self, stream: S) -> Result<(), Box<dyn std::error::Error>>;
}


#[async_trait]
impl<M> MessageEncoder for M
    where M: MessageKindTagged + Serialize + Send + Sized
{
    type Message = M;
    async fn write_to<'a, S>(self, mut stream: S) -> Result<(), Box<dyn std::error::Error>>
        where S: AsyncWriteExt + Unpin + Send
    {
        let mut message_buf = Vec::new();
        self.serialize(&mut Serializer::new(&mut message_buf))?;

        let header = MessageHeader {
            kind: self.kind().into(),
            length: message_buf.len() as u32,
        };
        let mut header_buf = Vec::new();
        header.serialize(&mut Serializer::new(&mut header_buf))?;

        stream.write_all(&header_buf).await?;
        stream.write_all(&message_buf).await?;

        Ok(())
    }
}

impl<'a> serde::Serialize for ResponseBody {
    fn serialize<S>(&self, serializer: S) -> Result<<S as serde::Serializer>::Ok, <S as serde::Serializer>::Error>
        where S: serde::Serializer
    {
        match self {
            ResponseBody::OkWithBuffer(x) => x.serialize(serializer),
            ResponseBody::OkWithHandle(x) => x.serialize(serializer),
            ResponseBody::Err(x) => x.serialize(serializer),
        }
    }
}


#[derive(Debug, Snafu)]
pub enum MessageReadError<M: std::error::Error + 'static> {
    #[snafu(display("Peer disconnected (gracefully): {:?}", peer_addr))]
    PeerDisconnectedGracefully { peer_addr: Option<SocketAddr> },

    #[snafu(display("Failed to read from socket: {}", source))]
    SocketReadError { source: std::io::Error },

    #[snafu(display("Message format error: {:?}", source))]
    BadMessageFormat { source: M }
}

#[async_trait]
pub trait MessageReader<S: AsyncReadExt + Unpin + Send> {
    async fn read_next<R: MessageDecoder>(
        &mut self,
        ctx: &mut BufferContext,
        peer_addr: Option<SocketAddr>)
        -> Result<R::Message, MessageReadError<R::Error>>;
}

#[async_trait]
impl<S: AsyncReadExt + Unpin + Send> MessageReader<S> for S {
    async fn read_next<R: MessageDecoder>(
        &mut self,
        ctx: &mut BufferContext,
        peer_addr: Option<SocketAddr>)
        -> Result<R::Message, MessageReadError<R::Error>>
    {
        read_next::<R, S>(self, ctx, peer_addr).await
    }
}

pub async fn read_next<R: MessageDecoder, S: AsyncReadExt + Unpin>(
    socket: &mut S,
    ctx: &mut BufferContext,
    peer_addr: Option<SocketAddr>)
    -> Result<R::Message, MessageReadError<R::Error>>
{
    let peer_addr_str = peer_addr.map_or("(unknown)".into(), |addr| addr.to_string());

    loop {
        let mut cur_state = ctx.get_state();
        if cur_state != BufferState::Done {
            // println!("[{}] (state = {:?}) waiting for read...", peer_addr_str, cur_state);
            let n = match socket.read_buf(&mut ctx.buffer).await {
                // socket closed
                Ok(n) if n == 0 => {
                    println!("[{}] peer disconnected", peer_addr_str);
                    (PeerDisconnectedGracefully { peer_addr }).fail()
                }
                Ok(n) => Ok(n),
                Err(e) => {
                    let context = SocketReadError;
                    Err(context.into_error(e))
                }
            }?;

            // println!("[{}] read {} bytes (len = {}, cap = {})", peer_addr_str, n, ctx.buffer.len(), ctx.buffer.capacity());

            if cur_state == BufferState::Empty || cur_state == BufferState::WaitHeader {
                let header_result = ctx.try_read_header();
                // println!("[read] header result = {:?}", header_result)
            }

            cur_state = ctx.get_state();
        }

        // Check if we have read the body at this point
        if cur_state == BufferState::Done {
            let target = ctx.header_with_target.as_ref().unwrap();
            let kind = &target.header.kind;
            // println!("[read] got data of {} bytes with message of kind {:?}", target.data_len(), kind);

            let message = R::read_from(ctx.buffer.borrow_mut(), &target)
                .context(BadMessageFormat)?;

            // Try reading the header to setup a subsequent call
            ctx.header_with_target = None;
            if ctx.buffer.len() > 0 {
                let header_result = ctx.try_read_header();
                // println!("[read] header result (post read) = {:?}", header_result)
            }

            // Grow the capacity back to the original buffer capacity
            ctx.regrow();

            return Ok(message);
        }
    }
}