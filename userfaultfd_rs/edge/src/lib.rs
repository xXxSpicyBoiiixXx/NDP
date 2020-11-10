use userfaultfd::{Uffd, UffdBuilder};
use nix::poll::{PollFd, PollFlags};
use std::ptr::null_mut;
use std::ptr;
use nix::sys::mman::{ProtFlags, MapFlags, mmap, munmap};
use std::os::unix::io::AsRawFd;
use std::sync::Arc;
use std::io::Cursor;

pub struct FaultRegistration {
    pub buf: MappedBufMut,
    pub uffd: Uffd,
}

impl FaultRegistration {
    pub fn new(buf: MappedBufMut) -> Result<Self, userfaultfd::Error> {
        let uffd = UffdBuilder::new()
            .close_on_exec(true)
            .non_blocking(true)
            .create()?;

        uffd.register(buf.ptr, buf.len)?;
        Ok(FaultRegistration { buf, uffd })
    }

    fn make_pollfd(&self) -> PollFd {
        PollFd::new(self.uffd.as_raw_fd(), PollFlags::POLLIN)
    }
}

pub mod util {
    use nix::unistd::{sysconf, SysconfVar};
    use nix::sys::mman::{mmap, ProtFlags, MapFlags};
    use std::ptr::null_mut;
    use libc::c_void;

    pub fn page_size() -> usize {
        let raw = sysconf(SysconfVar::PAGE_SIZE)
            .expect("Failed to determine system page size")
            .expect("System page size unavailable");

        if raw < 0 {
            panic!("System page size cannot be negative");
        }

        raw as usize
    }


    pub fn page_align_mut(ptr: *mut c_void, page_size: usize) -> *mut c_void {
        // equivalent to
        // #define PAGE_ALIGN(X, SIZE) ((X) & ~(SIZE - 1))
        //
        (ptr as usize & !(page_size as usize - 1)) as *mut c_void
    }
}

pub struct MappedBufMut {
    ptr: *mut libc::c_void,
    len: usize,
    page_size: usize,
}

// Force `MappedBufMut` to be `Send + Sync`-able across threads
unsafe impl Send for MappedBufMut { }
unsafe impl Sync for MappedBufMut { }

impl MappedBufMut {
    pub fn with_num_pages_of_size(num_pages: usize, page_size: usize) -> nix::Result<MappedBufMut> {
        let size = num_pages * page_size;

        let ptr = unsafe {
            mmap(ptr::null_mut(),
                 size,
                 ProtFlags::PROT_READ | ProtFlags::PROT_WRITE,
                 MapFlags::MAP_PRIVATE | MapFlags::MAP_ANONYMOUS,
                 -1,
                 0)?
        };

        Ok(MappedBufMut { ptr, len: size, page_size })
    }

    pub fn with_size(size: usize) -> nix::Result<MappedBufMut> {
        let page_size = util::page_size();
        let incr: usize = if (size % page_size) > 0 { 1 } else { 0 };
        let num_pages = (size / page_size) + incr;
        Self::with_num_pages_of_size(num_pages, page_size)
    }

    pub fn ptr(&self) -> *const libc::c_void {
        self.ptr
    }

    pub fn ptr_mut(&self) -> *mut libc::c_void {
        self.ptr
    }

    pub fn len(&self) -> usize {
        self.len
    }

    pub fn num_pages(&self) -> usize {
        self.len / self.page_size
    }

    pub fn as_slice_mut<'a>(&self) -> &'a mut [u8] {
        unsafe { std::slice::from_raw_parts_mut(self.ptr as *mut u8, self.len) }
    }

    pub fn as_cursor<'a>(&self) -> Cursor<&'a mut [u8]> {
        Cursor::new(self.as_slice_mut())
    }
}

impl Drop for MappedBufMut {
    fn drop(&mut self) {
        if self.ptr == null_mut() {
            return;
        }

        let result = unsafe { munmap(self.ptr, self.len) };
        result.expect("munmap: failed to unmap buffer");
    }
}

pub struct FaultHandlerMemorySpec {
    pub num_invoke_pages: usize,
    pub num_request_pages: usize,
    pub num_response_pages: usize,
}

impl FaultHandlerMemorySpec {
    pub fn ensure_or_panic(&self) {
        assert!(self.num_invoke_pages > 0, "Number of invoke pages must be non-zero");
        assert!(self.num_request_pages > 0, "Number of request pages must be non-zero");
        assert!(self.num_response_pages > 0, "Number of request pages must be non-zero");
    }
}

pub struct FaultHandlerMemory {
    pub page_size: usize,
    pub invoke_buf: Arc<MappedBufMut>,
    pub request_buf: Arc<MappedBufMut>,
    pub response_buf: Arc<MappedBufMut>,
}

impl FaultHandlerMemory {
    pub fn new(spec: FaultHandlerMemorySpec) -> nix::Result<FaultHandlerMemory> {
        spec.ensure_or_panic();
        let page_size = util::page_size();
        Ok(FaultHandlerMemory {
            page_size,
            invoke_buf: Arc::new(MappedBufMut::with_num_pages_of_size(spec.num_invoke_pages, page_size)?),
            request_buf: Arc::new(MappedBufMut::with_num_pages_of_size(spec.num_request_pages, page_size)?),
            response_buf: Arc::new(MappedBufMut::with_num_pages_of_size(spec.num_response_pages, page_size)?),
        })
    }
}

impl Clone for FaultHandlerMemory {
    fn clone(&self) -> Self {
        FaultHandlerMemory {
            page_size: self.page_size,
            invoke_buf: self.invoke_buf.clone(),
            request_buf: self.request_buf.clone(),
            response_buf: self.response_buf.clone(),
        }
    }
}

pub struct FnRegistrations {
    slots: Vec<usize>,
}

unsafe impl Send for FnRegistrations { }

pub type FnSlotId = u32;

impl FnRegistrations {
    pub fn with_max_registrations(max_registrations: FnSlotId) -> FnRegistrations {
        let mut v = Vec::new();
        v.resize(max_registrations as usize, 0);
        FnRegistrations {
            slots: v,
        }
    }

    pub fn set_handler<T, R>(&mut self, slot: FnSlotId, handler: extern "C" fn(T) -> R) {
        if (slot as usize) >= self.slots.len() {
            panic!("Function slot out of range");
        }
        self.slots[slot as usize] = unsafe { std::mem::transmute::<extern "C" fn(T) -> R, usize>(handler) };
    }

    pub fn handler_at(&self, slot: FnSlotId) -> Option<*const fn()> {
        let handler = self.slots[slot as usize];
        if handler == 0 {
            return None;
        }
        let f = unsafe { std::mem::transmute::<usize, *const fn()>(handler) };
        Some(f)
    }
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
