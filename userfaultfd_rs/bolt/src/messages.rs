use crate::message_kind;
use serde::{Deserialize, Serialize};

type PageHandle = u32;

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct FileInfo {
    pub name: String,
    pub size: Option<u64>,
}

impl FileInfo {
    pub fn new(name: String, size: Option<u64>) -> FileInfo {
        FileInfo { name, size }
    }
}

#[derive(Debug)]
pub enum RequestBody {
    Alloc(AllocRequest),
    Write(WriteRequest),
    Read(ReadRequest)
}

#[derive(Debug)]
pub enum ResponseBody {
    OkWithHandle(OkWithHandleResponse),
    OkWithBuffer(OkWithBufferResponse),
    Err(ErrResponse),
}

impl<'a> MessageKindTagged for ResponseBody {
    fn kind(&self) -> &'static str {
        match self {
            ResponseBody::OkWithBuffer(x) => x.kind(),
            ResponseBody::OkWithHandle(x) => x.kind(),
            ResponseBody::Err(x) => x.kind(),
        }
    }
}

impl From<OkWithHandleResponse> for ResponseBody {
    fn from(x: OkWithHandleResponse) -> Self {
        ResponseBody::OkWithHandle(x)
    }
}

impl<'a> From<OkWithBufferResponse> for ResponseBody {
    fn from(x: OkWithBufferResponse) -> Self {
        ResponseBody::OkWithBuffer(x)
    }
}

//

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct AllocRequest {
    pub size: u64,
}

impl MessageKindTagged for AllocRequest {
    fn kind(&self) -> &'static str { message_kind::REQUEST_ALLOC }
}

//

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct WriteRequest {
    pub handle: PageHandle,
    pub offset: u64,

    #[serde(with = "serde_bytes")]
    pub buf: Vec<u8>
}

impl MessageKindTagged for WriteRequest {
    fn kind(&self) -> &'static str { message_kind::REQUEST_WRITE }
}

//

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct ReadRequest {
    pub handle: PageHandle,
    pub offset: u64,
    pub len: u64,
}

impl MessageKindTagged for ReadRequest {
    fn kind(&self) -> &'static str { message_kind::REQUEST_READ }
}

//

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct OkWithHandleResponse {
    pub handle: PageHandle
}

impl<'a> MessageKindTagged for OkWithHandleResponse {
    fn kind(&self) -> &'static str { message_kind::RESPONSE_OK_HANDLE }
}

//

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct OkWithBufferResponse {
    pub handle: PageHandle,
    pub offset: u64,

    #[serde(with = "serde_bytes")]
    pub buf: Vec<u8>
}

impl<'a> MessageKindTagged for OkWithBufferResponse {
    fn kind(&self) -> &'static str { message_kind::RESPONSE_OK_HANDLE }
}

//

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub enum ErrorStatus {
    Unknown,
    InvalidHandle,
}

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct ErrResponse {
    pub error: ErrorStatus,
    pub message: String,
}

impl<'a> MessageKindTagged for ErrResponse {
    fn kind(&self) -> &'static str { message_kind::RESPONSE_ERR }
}


//

pub trait MessageKindTagged {
    fn kind(&self) -> &'static str;
}
