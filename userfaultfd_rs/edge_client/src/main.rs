extern crate rmp_serde as rmps;

use std::env;
use std::os::unix::io::AsRawFd;
use std::ptr;
use std::error::Error;
use std::sync::Arc;
use std::clone::Clone;
use std::borrow::{BorrowMut, Borrow};
use std::cell::RefCell;
use std::thread;
use std::convert::TryInto;
use std::io::Cursor;

use nix::poll::{poll, PollFd, PollFlags};
use nix::sys::mman::{mmap, MapFlags, ProtFlags};
use nix::unistd::{sysconf, SysconfVar};
use libc::{self, c_void};

use userfaultfd::{Event, Uffd, UffdBuilder};
use byteorder::{WriteBytesExt, NativeEndian, ReadBytesExt};
use edge::{FaultHandlerMemory, FaultHandlerMemorySpec, FaultRegistration, FnRegistrations, FnSlotId, MappedBufMut, util};
use bolt::buffer::BufferContext;
use std::net::SocketAddr;
use tokio::net::TcpStream;
use bolt::codec::{MessageReader, MessageEncoder};
use bolt::messages::{ResponseBody, PageHandle, MessageKindTagged, ResponseBodyOwned};
use snafu::{Snafu, ResultExt};
use log::{trace, debug, info, error};
use tokio::task::{block_in_place, JoinHandle};
use tokio::runtime::Runtime;
use bytes::{BytesMut, BufMut, Buf, Bytes};
use std::fmt::Write;
use rand::{thread_rng, RngCore};
use std::time::Instant;
use tokio::time::Duration;
use humansize::{FileSize, file_size_opts};
use std::cmp::min;
use tokio::sync::Mutex;

#[derive(Debug, Snafu)]
enum CommandError {
    #[snafu(display("Bad argument to command: {}", reason))]
    BadArgument { reason: String },

    #[snafu(display("Failed to parse address: {}", source))]
    BadAddressFormat { source: std::net::AddrParseError },

    #[snafu(display("Unable to resolve address: {}", source))]
    AddressResolveFailed { source: std::io::Error },

    #[snafu(display("No addresses were associated with the host"))]
    NoAddressFound,

    #[snafu(display("No file path was specified in the URL"))]
    UrlMissingPath,

    #[snafu(display("Failed to connect to remote host: {}", source))]
    ConnectionError { source: std::io::Error },

    #[snafu(display("Failed to send message to remote host: {}", source))]
    SendError { source: std::io::Error },

    #[snafu(display("Failed to receive message to remote host: {}", source))]
    ReceiveError { source: bolt::codec::MessageReadError<rmps::decode::Error> },

    #[snafu(display("Failed to encode message: {}", source))]
    MessageEncodingError { source: Box<dyn std::error::Error> },

    #[snafu(display("Response was invalid or unexpected: {}", reason))]
    BadResponseError { reason: String },

    #[snafu(display("Unable to write to download file: {}", source))]
    DownloadIoError { source: std::io::Error },
}

async fn alloc_example() -> Result<PageHandle, CommandError> {
    let mut ctx = BufferContext::new(8 * 1024);

    let address = "127.0.0.1:9090".parse::<SocketAddr>().context(BadAddressFormat)?;

    let mut client = TcpStream::connect(address).await
        .context(ConnectionError)?;

    let peer_address = client.borrow().peer_addr().ok();

    let req = bolt::messages::AllocRequest { size: 4096, };
    req.write_to(&mut client).await
        .context(MessageEncodingError)?;

    info!("waiting on response...");
    let resp_any = client.borrow_mut().read_next::<ResponseBodyOwned>(&mut ctx, peer_address).await
        .context(ReceiveError)?;

    debug!("alloc got {:?}", resp_any);
    let handle = if let ResponseBodyOwned::OkWithHandle(resp) = resp_any {
        let handle = resp.handle;
        println!("got handle id = {}", handle);
        Ok(handle)
    } else {
        let err = BadResponseError { reason: format!("Wrong response type. Got '{:?}'", resp_any) };
        return err.fail().map_err(|x| x.into());
    }?;

    let mut rand = thread_rng();
    let mut buf = BytesMut::with_capacity(4096);
    while buf.len() < buf.capacity() {
        buf.put_u64(rand.next_u64())
    }

    let req = bolt::messages::WriteRequestRef::<'_> {
        handle,
        buf: serde_bytes::Bytes::new(buf.bytes()),
        offset: 0
    };
    debug!("write...");
    req.write_to(&mut client).await.context(MessageEncodingError)?;

    let resp = client.borrow_mut().read_next::<ResponseBodyOwned>(&mut ctx, peer_address).await
        .context(ReceiveError)?;
    debug!("write got {:?}", resp);

    let req = bolt::messages::ReadRequest {
        handle,
        offset: 0,
        len: 4096
    };
    debug!("read...");
    req.write_to(&mut client).await.context(MessageEncodingError)?;

    let resp = client.borrow_mut().read_next::<ResponseBodyOwned>(&mut ctx, peer_address).await
        .context(ReceiveError)?;

    let buf_recv = if let ResponseBodyOwned::OkWithBuffer(resp) = resp {
        debug!("read got {} bytes", resp.buf.len());
        Ok(resp.buf)
    }
    else if let ResponseBodyOwned::Err(err) = resp {
        let err = BadResponseError { reason: format!("Server-side error. ({:?}) {}", err.error, err.message) };
        return err.fail().map_err(|x| x.into());
    }
    else {
        let err = BadResponseError { reason: format!("Wrong response type. Got '{}'", &resp.kind()) };
        return err.fail().map_err(|x| x.into());
    }?;

    let check = buf.to_vec() == buf_recv.to_vec();
    if !check {
        error!("Fail read-write check!");
        let err = BadResponseError { reason: format!("Bad response. Read did not match write.") };
        return err.fail().map_err(|x| x.into());
    }

    Ok(handle)
}


async fn write_read_throughput() -> Result<PageHandle, CommandError> {
    let mb: usize = 4096 * 256;
    let gb: usize = 4096 * 4096 * 64;
    let buf_size: usize = 512 * mb;

    let chunk_num_pages = 64; // MiB
    let chunk_size: usize = chunk_num_pages * 4096;

    //

    let mut ctx = BufferContext::new((chunk_num_pages + 4) * 4096);

    // Change the IP addresses for desired server hosting
    let address = "192.168.50.239:9090".parse::<SocketAddr>().context(BadAddressFormat)?;

    let mut client = TcpStream::connect(address).await
        .context(ConnectionError)?;

    let peer_address = client.borrow().peer_addr().ok();

    //Buffer size for allocation
    let req = bolt::messages::AllocRequest { size: buf_size as u64, };
    req.write_to(&mut client).await
        .context(MessageEncodingError)?;

    let resp_any = client.borrow_mut().read_next::<ResponseBodyOwned>(&mut ctx, peer_address).await
        .context(ReceiveError)?;

    let handle = if let ResponseBodyOwned::OkWithHandle(resp) = resp_any {
        Ok(resp.handle)
    } else {
        let err = BadResponseError { reason: format!("Wrong response type. Got '{:?}'", resp_any) };
        return err.fail().map_err(|x| x.into());
    }?;

    let mut buf = BytesMut::with_capacity(buf_size);
    buf.resize(buf_size, 0);

    let num_iters = 1;
    let mut durations = Vec::with_capacity(num_iters);

    {
        let mut rand = thread_rng();
        rand.fill_bytes(&mut buf);
    }

    let (read_owned, write_owned) = client.into_split();
    let read = Arc::new(Mutex::new(read_owned));
    let write = Arc::new(Mutex::new(write_owned));

    for _ in 0..num_iters {
        let mut jobs: Vec<JoinHandle<_>> = Vec::new();
        for i in (0..buf_size).step_by(chunk_size) {
            jobs.push(tokio::spawn(async {
                foo(i, chunk_size, buf_size, &buf, handle, &mut client, &mut ctx, peer_address, &mut durations).await.unwrap();
                async fn foo(
                    i: usize,
                    chunk_size: usize,
                    buf_size: usize,
                    buf: &BytesMut,
                    handle: PageHandle,
                    mut client: &mut TcpStream,
                    mut ctx: &mut BufferContext,
                    peer_address: Option<SocketAddr>,
                    durations: &mut Vec<Duration>,
                ) -> Result<(), CommandError> {
                    let start = Instant::now();

                    // debug!("write {}", i);
                    let j = min(i + chunk_size, buf_size);
                    let chunk = &buf[i..j];
                    let req = bolt::messages::WriteRequestRef {
                        handle,
                        buf: chunk,
                        offset: i as u64,
                    };
                    req.write_to(&mut client).await.context(MessageEncodingError)?;

                    let resp = client.borrow_mut().read_next::<ResponseBodyOwned>(&mut ctx, peer_address).await
                        .context(ReceiveError)?;

                    // debug!("read {}", i);
                    let req = bolt::messages::ReadRequest {
                        handle,
                        offset: i as u64,
                        len: (j - i) as u64
                    };
                    req.write_to(&mut client).await.context(MessageEncodingError)?;

                    let resp = client.borrow_mut().read_next::<ResponseBodyOwned>(&mut ctx, peer_address).await
                        .context(ReceiveError)?;

                    let chunk_recv = if let ResponseBodyOwned::OkWithBuffer(resp) = resp {
                        Ok(resp.buf)
                    } else if let ResponseBodyOwned::Err(err) = resp {
                        let err = BadResponseError { reason: format!("Server-side error. ({:?}) {}", err.error, err.message) };
                        return err.fail().map_err(|x| x.into());
                    } else {
                        let err = BadResponseError { reason: format!("Wrong response type. Got '{}'", &resp.kind()) };
                        return err.fail().map_err(|x| x.into());
                    }?;

                    let end = Instant::now();
                    durations.push(end - start);

                    let check = chunk == chunk_recv;
                    if !check {
                        error!("Fail read-write check!");
                        let err = BadResponseError { reason: format!("Bad response. Read did not match write.") };
                        return err.fail().map_err(|x| x.into());
                    }

                    Ok(())
                };
            }));
        }
    }

    // for x in durations.iter() {
    //     println!("{}", x.as_nanos());
    // }

    let sum: Duration = durations.iter().sum();
    let total_bytes = num_iters * buf_size;
    let total_secs = sum.as_secs_f64();
    let speed = ((total_bytes as f64) / total_secs) as u64;
    println!("{} in {:.4} s (~{} / sec)",
             total_bytes.file_size(file_size_opts::BINARY).unwrap(),
             total_secs,
             speed.file_size(file_size_opts::BINARY).unwrap(),
    );

    Ok(handle)
}



fn fault_handler_thread(ctx: ResolverContext) {
    let page_size = ctx.faulting.mem.page_size;
    let num_response_pages = ctx.faulting.mem.response_buf.num_pages();
    let tmp = MappedBufMut::with_num_pages_of_size(num_response_pages, page_size).unwrap();

    let uffd = &*ctx.faulting.uffd;
    loop {
        // See what poll() tells us about the userfaultfd
        let pollfd = PollFd::new(uffd.as_raw_fd(), PollFlags::POLLIN);
        let nready = poll(&mut [pollfd], -1).expect("poll");

        // Read event from file descriptor
        let event = ctx.faulting.uffd
            .read_event()
            .expect("Failed to read uffd_msg")
            .expect("No uffd_msg available to read");

        if let Event::Pagefault { addr: faulting_addr, .. } = event {
            let mut input = ctx.faulting.mem.request_buf.as_cursor();
            let slot_id: u32 = input.read_u32::<NativeEndian>().unwrap();

            let handler_raw = ctx.fns.handler_at(slot_id).expect(format!("Unhandled function slot id = {}", slot_id).as_str()); // HACK: might hang
            let handle = unsafe { std::mem::transmute::<*const fn(), extern "cdecl" fn(u32) -> i64>(handler_raw) };

            let arg0 = input.read_u32::<NativeEndian>().unwrap();
            let result: i64 = handle(arg0);

            let mut output = ctx.faulting.mem.response_buf.as_cursor();
            output.write_i64::<NativeEndian>(result).expect("Failed to write value");

            let _num_copied = unsafe {
                ctx.respond_with_wake(faulting_addr, &tmp)
                    .expect("uffd copy")
            };
        } else {
            panic!("Unexpected event on userfaultfd");
        }
    }
}

struct FaultingContext {
    mem: FaultHandlerMemory,
    uffd: Arc<Uffd>
}

struct ResolverContext {
    faulting: FaultingContext,
    fns: FnRegistrations,
}

impl ResolverContext {
    pub fn new(faulting: FaultingContext, max_registrations: FnSlotId) -> ResolverContext {
        ResolverContext {
            faulting,
            fns: FnRegistrations::with_max_registrations(max_registrations),
        }
    }

    pub unsafe fn respond_with_wake(&self, faulting_addr: *mut c_void, resolved: &MappedBufMut) -> userfaultfd::Result<usize> {
        let faulting_page_addr = util::page_align_mut(faulting_addr, self.faulting.mem.page_size);
        self.faulting.uffd.copy(resolved.ptr(), faulting_page_addr, resolved.len(), true)
    }
}

impl FaultingContext {
    fn make_uffd(buf: &MappedBufMut) -> Result<Uffd, userfaultfd::Error> {
        let uffd = UffdBuilder::new()
            .non_blocking(true)
            .close_on_exec(true)
            .create()?;

        uffd.register(buf.ptr_mut() as *mut c_void, buf.len())?;
        Ok(uffd)
    }

    fn new(spec: FaultHandlerMemorySpec) -> Result<FaultingContext, Box<dyn Error>> {
        let mem = FaultHandlerMemory::new(spec)?;
        let uffd = Self::make_uffd(&*mem.invoke_buf)?;
        Ok(FaultingContext {
            mem, uffd: Arc::new(uffd)
        })
    }
}

impl Clone for FaultingContext {
    fn clone(&self) -> Self {
        FaultingContext {
            mem: self.mem.clone(),
            uffd: self.uffd.clone(),
        }
    }
}

const EDGE_FIB_SLOT: FnSlotId = 42;
extern "C" fn fib_kernel(n: u32) -> i64 {
    let mut n1: i64 = -1 as i64;
    let mut n2: i64 = 1;
    for _ in 0..=n {
        let sum = n1 + n2;
        n1 = n2;
        n2 = sum;
    }
    println!("kernel! fib({}) = {}", n, n2);
    n2
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    pretty_env_logger::init();

    let mem_spec = FaultHandlerMemorySpec {
        num_invoke_pages: 1,
        num_request_pages: 1,
        num_response_pages: 1,
    };

    let ctx = FaultingContext::new(mem_spec)?;
    let mut resolver_ctx = ResolverContext::new(ctx.clone(), 1000);
    resolver_ctx.fns.set_handler(EDGE_FIB_SLOT, fib_kernel);

    let _s = std::thread::spawn(move || fault_handler_thread(resolver_ctx));

    let fib_n = 10;
    let mut request_buf = ctx.mem.request_buf.as_cursor();
    request_buf.write_u32::<NativeEndian>(EDGE_FIB_SLOT)?;
    request_buf.write_u32::<NativeEndian>(fib_n)?;

    let ptr = (*ctx.mem.invoke_buf).ptr_mut() as *mut u8;
    let _poke = unsafe { *ptr };

    let mut response_buf = ctx.mem.response_buf.as_cursor();
    let result =  response_buf.read_i64::<NativeEndian>().unwrap();
    println!("user  ! fib({}) = {}", fib_n, result);

    //

    // let handle = alloc_example().await.unwrap();
    let mut handles = Vec::new();
    for _ in 0..16 {
        handles.push(tokio::spawn(async { write_read_throughput().await.unwrap() }));
    }
    futures::future::join_all(handles).await;

    Ok(())
}
