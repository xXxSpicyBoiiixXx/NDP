use tokio::net::{TcpListener, TcpStream};
use tokio::sync::{Mutex, RwLock};
use std::sync::{Arc};
use bolt::buffer::BufferContext;
use std::ops::{DerefMut, Deref};
use bolt::codec::{MessageReader, MessageEncoder};
use bolt::messages;
use tokio::task;
use bolt::messages::{RequestBody, OkWithHandleResponse, ErrResponse, ErrorStatus, MessageKindTagged, OkWithBufferResponse};
use edge::{MappedBufMut, FnRegistrations};
use vec_arena::Arena;
use bolt::messages::ResponseBody::OkWithHandle;
use std::io::{Write, Read};
use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};
use std::error::Error;
use byteorder::{ReadBytesExt, NativeEndian};

pub struct ServerContext {
    pages: Arc<RwLock<Arena<MappedBufMut>>>,
    fns: Arc<RwLock<FnRegistrations>>,
}

impl ServerContext {
    pub fn with_capacity(handles_initial_capacity: usize, max_num_fns: u32) -> ServerContext {
        ServerContext {
            pages: Arc::new(RwLock::new(Arena::<MappedBufMut>::with_capacity(handles_initial_capacity))),
            fns: Arc::new(RwLock::new(FnRegistrations::with_max_registrations(max_num_fns))),
        }
    }

    fn consume_into_client(&self, socket: TcpStream) -> ClientHandlerContext {
        let (socket_read, socket_write) = socket.into_split();
        let socket_read = Arc::new(Mutex::new(socket_read));
        let socket_write = Arc::new(Mutex::new(socket_write));
        ClientHandlerContext {
            pages: self.pages.clone(),
            socket_read,
            socket_write,
            fns: self.fns.clone(),
        }
    }
}

#[derive(Clone)]
struct ClientHandlerContext {
    pages: Arc<RwLock<Arena<MappedBufMut>>>,
    socket_read: Arc<Mutex<OwnedReadHalf>>,
    socket_write: Arc<Mutex<OwnedWriteHalf>>,
    fns: Arc<RwLock<FnRegistrations>>,
}

impl ClientHandlerContext {

    async fn reply_bad_handle(&self) -> Result<(), Box<dyn Error>> {
        let response = ErrResponse { error: ErrorStatus::InvalidHandle, message: "Invalid handle".into() };
        self.reply(response).await?;
        Ok(())
    }

    async fn reply<M: MessageEncoder>(&self, message: M) -> Result<(), Box<dyn Error>> {
        let mut lock = self.socket_write.lock().await;
        let socket = lock.deref_mut();
        message.write_to(socket).await?;
        Ok(())
    }
}

pub async fn run(listener: &mut TcpListener, ctx: ServerContext) -> Result<(), Box<dyn std::error::Error>> {
    loop {
        let (mut socket_raw, peer_address) = listener.accept().await?;
        let client = ctx.consume_into_client(socket_raw);
        tokio::spawn(async move {
            let client = client;
            let mut buf_ctx = BufferContext::new(32 * 1024);
            loop {
                let mut lock = client.socket_read.lock().await;
                let result = lock.deref_mut().read_next::<messages::RequestBody>(&mut buf_ctx, peer_address.into()).await;
                drop(lock);

                if let Ok(body) = result {
                    // println!("[server] received from client '{}'", body.kind());

                    match body {
                        RequestBody::Alloc(alloc) => handle_alloc(alloc, client.clone()).await.unwrap(),
                        RequestBody::Write(write) => handle_write(write, client.clone()).await.unwrap(),
                        RequestBody::Read(read) => handle_read(read, client.clone()).await.unwrap(),
                        RequestBody::Invoke(invoke) => handle_invoke(invoke, client.clone()).await.unwrap(),
                    }
                } else {
                    println!("[server] shutdown client handler");
                    break;
                }
            }
        });
    }
}

async fn handle_invoke(invoke: messages::InvokeRequest, client: ClientHandlerContext) -> Result<(), Box<dyn Error>> {
     let request_buf_handle = invoke.handle;
     let pages_lock = client.pages.read().await;
     let page_buf = pages_lock.deref().get(request_buf_handle as usize);
     match page_buf {
       None => client.reply_bad_handle().await?,
           Some(request_buf) => {
           let mut input = request_buf.as_cursor();
           let slot_id: u32 = input.read_u32::<NativeEndian>().unwrap();

            let handler_raw = client.fns.handler_at(slot_id).expect(format!("Unhandled function slot id = {}", slot_id).as_str()); // HACK: might hang
            let handle = unsafe { std::mem::transmute::<*const fn(), extern "cdecl" fn(u32) -> i64>(handler_raw) };

             let arg0 = input.read_u32::<NativeEndian>().unwrap();
             let result: i64 = handle(arg0);

            let mut output = ctx.faulting.mem.response_buf.as_cursor();
            output.write_i64::<NativeEndian>(result);
        }
     }
    Ok(())
}

async fn handle_read(read: messages::ReadRequest, client: ClientHandlerContext) -> Result<(), Box<dyn Error>> {
    let lock = client.pages.read().await;
    let page_buf = lock.deref().get(read.handle as usize);
    match page_buf {
        Some(page_buf) => {
            if (read.offset + read.len as u64) > (page_buf.len() as u64) {
                let mut lock = client.socket_write.lock().await;
                let socket = lock.deref_mut();
                let response = ErrResponse { error: ErrorStatus::ArgumentOutOfRange, message: "Offset is out of range".into() };
                response.write_to(socket).await?;
                return Ok(());
            }

            let mut output: Vec<u8> = Vec::new();
            output.resize(read.len as usize, 0);
            let mut page_cursor = page_buf.as_cursor();
            page_cursor.set_position(read.offset);
            page_cursor.read_exact(output.as_mut_slice());

            {
                let mut lock = client.socket_write.lock().await;
                let socket = lock.deref_mut();
                let response = OkWithBufferResponse {
                    handle: read.handle,
                    offset: read.offset,
                    buf: output,
                };
                response.write_to(socket).await?;
            }
        }
        None => client.reply_bad_handle().await?,
    };

    Ok(())
}


async fn handle_write(write: messages::WriteRequest, client: ClientHandlerContext) -> Result<(), Box<dyn Error>> {
    let lock = client.pages.read().await;
    let page_buf = lock.deref().get(write.handle as usize);
    match page_buf {
        Some(page_buf) => {
            if (write.offset + write.buf.len() as u64) > (page_buf.len() as u64) {
                let mut lock = client.socket_write.lock().await;
                let socket = lock.deref_mut();
                let response = ErrResponse { error: ErrorStatus::ArgumentOutOfRange, message: "Offset is out of range".into() };
                response.write_to(socket).await?;
                return Ok(());
            }

            let mut page_cursor = page_buf.as_cursor();
            page_cursor.set_position(write.offset);
            page_cursor.write(write.buf.as_slice());

            {
                let mut lock = client.socket_write.lock().await;
                let socket = lock.deref_mut();
                let response = OkWithHandleResponse { handle: write.handle };
                response.write_to(socket).await?;
            }
        }
        None => client.reply_bad_handle().await?,
    };

    Ok(())
}

async fn handle_alloc(alloc: messages::AllocRequest, client_ctx: ClientHandlerContext) -> Result<(), Box<dyn Error>> {
    let handle = {
        let mut lock = client_ctx.pages.write().await;
        lock.deref_mut().insert(MappedBufMut::with_size(alloc.size as usize)?) as u64
    };

    let response = OkWithHandleResponse { handle };

    {
        let mut lock = client_ctx.socket_write.lock().await;
        response.write_to(lock.deref_mut()).await?;
    }

    Ok(())
}
