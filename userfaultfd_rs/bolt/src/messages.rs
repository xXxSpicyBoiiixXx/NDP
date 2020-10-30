use crate::message_kind;
use serde::{Deserialize, Serialize};

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
    Fetch(FileFetchRequest),
    Listing(FileListingRequest)
}

#[derive(Debug)]
pub enum ResponseBody {
    Chunk(FileChunkResponse),
    Listing(FileListingResponse)
}

impl From<FileChunkResponse> for ResponseBody {
    fn from(x: FileChunkResponse) -> Self {
        ResponseBody::Chunk(x)
    }
}

impl<'a> From<FileListingResponse> for ResponseBody {
    fn from(x: FileListingResponse) -> Self {
        ResponseBody::Listing(x)
    }
}


#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct FileFetchRequest {
    pub name: String,
}

impl MessageKindTagged for FileFetchRequest {
    fn kind(&self) -> &'static str { message_kind::REQUEST_FETCH }
}

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct FileChunkResponse {
    pub path: String,
    pub offset: u64,
    pub size: u64,

    #[serde(with = "serde_bytes")]
    pub buffer: Vec<u8>,
}

impl<'a> MessageKindTagged for FileChunkResponse {
    fn kind(&self) -> &'static str { message_kind::RESPONSE_CHUNK }
}

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct FileListingRequest {
    dummy: ()
}

impl MessageKindTagged for FileListingRequest {
    fn kind(&self) -> &'static str { message_kind::REQUEST_LISTING }
}

impl FileListingRequest {
    pub fn new() -> FileListingRequest {
        FileListingRequest { dummy: () }
    }
}

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct DhtAddNodeRequest {
    pub files: Vec<FileInfo>,
}

#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct DhtAddNodeResponse {
    dummy: (),
}


#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct DhtRemoveNodeRequest {
    pub files: Vec<FileInfo>,
}


#[derive(Debug, PartialEq, Deserialize, Serialize)]
pub struct FileListingResponse {
    pub files: Vec<FileInfo>,
}

impl MessageKindTagged for FileListingResponse {
    fn kind(&self) -> &'static str {
        message_kind::RESPONSE_LISTING
    }
}

impl<'a> MessageKindTagged for ResponseBody {
    fn kind(&self) -> &'static str {
        match self {
            ResponseBody::Chunk(x) => x.kind(),
            ResponseBody::Listing(x) => x.kind(),
        }
    }
}

pub trait MessageKindTagged {
    fn kind(&self) -> &'static str;
}


