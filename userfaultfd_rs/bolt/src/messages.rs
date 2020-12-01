use crate::message_kind;
use serde::{Deserialize, Serialize};
use std::path::Display;

pub type PageHandle = u64;

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
pub enum RequestBody<'a> {
    Alloc(AllocRequest),
    Write(WriteRequestRef<'a>),
    Read(ReadRequest),
    Invoke(InvokeRequest),
}

#[derive(Debug)]
pub enum RequestBodyOwned {
    Alloc(AllocRequest),
    Write(WriteRequestOwned),
    Read(ReadRequest),
    Invoke(InvokeRequest),
}

impl<'a> MessageKindTagged for RequestBody<'a> {
    fn kind(&self) -> &'static str {
        match self {
            Self::Alloc(x) => x.kind(),
            Self::Write(x) => x.kind(),
            Self::Read(x) => x.kind(),
            Self::Invoke(x) => x.kind(),
        }
    }
}

impl MessageKindTagged for RequestBodyOwned {
    fn kind(&self) -> &'static str {
        match self {
            Self::Alloc(x) => x.kind(),
            Self::Write(x) => x.kind(),
            Self::Read(x) => x.kind(),
            Self::Invoke(x) => x.kind(),
        }
    }
}

//

#[derive(Debug)]
pub enum ResponseBody<'a> {
    OkWithHandle(OkWithHandleResponse),
    OkWithBuffer(OkWithBufferResponse<'a>),
    Err(ErrResponse),
}

#[derive(Debug)]
pub enum ResponseBodyOwned {
    OkWithHandle(OkWithHandleResponse),
    OkWithBuffer(OkWithBufferResponseOwned),
    Err(ErrResponse),
}

impl<'a> MessageKindTagged for ResponseBody<'a> {
    fn kind(&self) -> &'static str {
        match self {
            Self::OkWithBuffer(x) => x.kind(),
            Self::OkWithHandle(x) => x.kind(),
            Self::Err(x) => x.kind(),
        }
    }
}

impl<'a> MessageKindTagged for ResponseBodyOwned {
    fn kind(&self) -> &'static str {
        match self {
            Self::OkWithBuffer(x) => x.kind(),
            Self::OkWithHandle(x) => x.kind(),
            Self::Err(x) => x.kind(),
        }
    }
}

//

impl<'a> From<OkWithHandleResponse> for ResponseBody<'a> {
    fn from(x: OkWithHandleResponse) -> Self {
        ResponseBody::OkWithHandle(x)
    }
}

impl<'a> From<OkWithBufferResponse<'a>> for ResponseBody<'a> {
    fn from(x: OkWithBufferResponse<'a>) -> Self {
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

#[derive(Debug, PartialEq, Serialize)]
pub struct WriteRequestRef<'a> {
    pub handle: PageHandle,
    pub offset: u64,

    #[serde(with = "serde_bytes")]
    pub buf: &'a [u8],
}

#[derive(Debug, PartialEq, Deserialize)]
pub struct WriteRequestOwned {
    pub handle: PageHandle,
    pub offset: u64,

    #[serde(with = "serde_bytes")]
    pub buf: Vec<u8>,
}

impl<'a> MessageKindTagged for WriteRequestRef<'a> {
    fn kind(&self) -> &'static str { message_kind::REQUEST_WRITE }
}

impl MessageKindTagged for WriteRequestOwned {
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
pub struct InvokeRequest {
    pub handle: PageHandle,
}

impl MessageKindTagged for InvokeRequest {
    fn kind(&self) -> &'static str { message_kind::REQUEST_INVOKE }
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

#[derive(Debug, PartialEq, Serialize)]
pub struct OkWithBufferResponse<'a> {
    pub handle: PageHandle,
    pub offset: u64,

    #[serde(with = "serde_bytes")]
    pub buf: &'a [u8],
}

#[derive(Debug, PartialEq, Deserialize)]
pub struct OkWithBufferResponseOwned {
    pub handle: PageHandle,
    pub offset: u64,

    #[serde(with = "serde_bytes")]
    pub buf: Vec<u8>,
}

impl<'a> MessageKindTagged for OkWithBufferResponse<'a> {
    fn kind(&self) -> &'static str { message_kind::RESPONSE_OK_BUFFER }
}

impl<'a> MessageKindTagged for OkWithBufferResponseOwned {
    fn kind(&self) -> &'static str { message_kind::RESPONSE_OK_BUFFER }
}

//

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub enum ErrorStatus {
    Unknown,
    InvalidHandle,
    ArgumentOutOfRange
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
