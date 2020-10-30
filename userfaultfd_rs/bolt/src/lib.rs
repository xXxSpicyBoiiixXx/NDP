extern crate serde;
extern crate rmp_serde as rmps;
extern crate tokio_util;

use std::io::{Cursor};
use serde::{Deserialize, Serialize};
use bytes::{BytesMut};
use rmps::{Serializer};
use rmps::decode::Error;
use tokio_util::codec::Decoder;
use serde::export::PhantomData;
use tokio::prelude::io::AsyncWriteExt;
use async_trait::async_trait;

pub mod messages;
pub mod codec;
pub mod buffer;
pub mod nodes;

pub mod message_kind {
    pub const REQUEST_ALLOC: &str = "ALLOC";
    pub const REQUEST_WRITE: &str = "WRITE";
    pub const REQUEST_READ: &str = "READ";
    pub const RESPONSE_OK_HANDLE: &str = "OK_H";
    pub const RESPONSE_OK_BUFFER: &str = "OK_B";
    pub const RESPONSE_ERR: &str = "ERR";
}

#[cfg(test)]
mod tests {
    use rmps::Serializer;
    use serde::Serialize;
    use std::io::Cursor;
    use crate::messages::FileInfo;

    #[test]
    fn it_works() {
        let mut message_buf = Vec::new();
        let expected = FileInfo {
            name: "/foo.txt".into(),
            size: Some(42),
        };

        expected.serialize(&mut Serializer::new(&mut message_buf)).unwrap();

        let reader = Cursor::new(message_buf);
        let actual = rmps::from_read::<_, FileInfo>(reader).unwrap();
        assert_eq!(expected, actual);
    }
}
