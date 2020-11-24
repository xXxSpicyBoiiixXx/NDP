use std::io::Cursor;
use bytes::{BytesMut, Buf};
use crate::codec::{MessageHeaderDecoded, MessageHeader};

#[derive(PartialEq, Debug)]
pub enum BufferState {
    Empty,
    WaitHeader,
    WaitData,
    Done,
}

pub struct BufferContext {
    pub(crate) initial_capacity: usize,
    pub(crate) buffer: BytesMut,
    pub(crate) header_with_target: Option<MessageHeaderDecoded>,
}

impl BufferContext {
    pub fn new(capacity: usize) -> BufferContext {
        BufferContext {
            initial_capacity: capacity,
            buffer: BytesMut::with_capacity(capacity),
            header_with_target: None,
        }
    }

    pub fn regrow(&mut self) {
        let cap = self.buffer.capacity();
        if let Some(extra) = self.initial_capacity.checked_sub(cap) {
            self.buffer.reserve(extra);
        }
    }

    pub fn header(&self) -> Option<&MessageHeader> {
        self.header_with_target.as_ref().map(|x| &x.header)
    }

    pub fn set_header_with_len(&mut self, header: MessageHeader, header_len: u32) {
        self.header_with_target = Some(MessageHeaderDecoded { header, header_len });
    }

    pub fn try_read_header(&mut self) -> Result<(), rmps::decode::Error> {
        let mut cursor = Cursor::new(&self.buffer);

        let parse_result = rmps::from_read::<_, MessageHeader>(&mut cursor);
        let result = match parse_result {
            Ok(header) => {
                let header_len = cursor.position() as u32;
                // println!("[read] header len = {} bytes | {:?}", header_len, header);
                self.set_header_with_len(header, header_len);
                self.buffer.advance(header_len as usize);
                Ok(())
            }
            Err(e) => Err(e)
        };

        result
    }

    pub fn get_state(&self) -> BufferState {
        let pos = self.buffer.len() as u32;

        let state = match (pos, &self.header_with_target) {
            (0, None) => BufferState::Empty,
            (_, None) => BufferState::WaitHeader,
            (pos, Some(h)) if pos >= h.header.length => BufferState::Done,
            (_, Some(_)) => BufferState::WaitData,
        };

        // println!("[buf] state = {:?} | len = {} | cap = {}", state, self.buffer.len(), self.buffer.capacity());
        state
    }
}
