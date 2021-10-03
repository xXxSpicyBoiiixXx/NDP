use std::io;
use std::process::abort;
use std::rc::{Rc};
use std::cell::RefCell;
use rand::prelude::*;
extern crate clap;
use clap::{Arg, crate_version, App};
use sha2::{Sha256, Digest};

const BLOCK_SIZE : usize = 4096;


struct MerkleNode {
    hash: Vec<u8>,
    start_offset: usize,
    blob_len: usize,
    left: Option<Rc<Box<MerkleNode>>>,
    right: Option<Rc<Box<MerkleNode>>>,
}

pub struct MerkleTree<T: SetGettable> {
    data: Rc<RefCell<T>>,
    root: Box<MerkleNode>,
    height: u8,
}

impl MerkleNode {
    fn new<T: SetGettable>(data: &T, level: usize, size: usize, offset: usize) -> Box<Self> {

        if size == BLOCK_SIZE {
            Box::new(MerkleNode {
                hash: data.hash(offset, size).to_vec(),
                start_offset: offset,
                blob_len: size,
                left: None,
                right: None,
            })
        } else {

            let left = Rc::new(MerkleNode::new(
                            data,
                            level + 1,
                            size / 2,
                            offset));

            let right = Rc::new(MerkleNode::new(
                            data,
                            level + 1,
                            size / 2,
                            offset + size/2));


            let ltmp = &*Rc::clone(&left);
            let rtmp = &*Rc::clone(&right);
            Box::new(MerkleNode {
                hash: ltmp.hash_concat(rtmp),
                start_offset: offset,
                blob_len: size,
                left: Some(Rc::clone(&left)),
                right: Some(Rc::clone(&right)),
            })
        }
    }

    fn hash_concat(&self, other: &MerkleNode) -> Vec<u8> {
        concat(self.hash.as_slice(), other.hash.as_slice())
    }


}

fn concat<'a>(left: &'a[u8], right: &'a[u8]) -> Vec<u8> {
    [left, right].concat()
}


impl<T: SetGettable> MerkleTree<T> {
    fn new(data: T) -> Self {
        let sz = data.size();
        let d = Rc::new(RefCell::new(data));
        MerkleTree {
            data: d.clone(),
            root: MerkleNode::new(&*d.clone().borrow(), 0, sz, 0),
            height: (data.size()/BLOCK_SIZE).log2(),
        }
    }

    fn set(&mut self, idx: usize, val: u8) {
        self.data.borrow_mut().set(idx, val);
        self.update(idx);
    }

    fn get(&self, idx: usize) -> u8 {
        self.data.borrow().get(idx)
    }

    fn update(&mut self, idx: usize) {
        // TODO
        let block_no = idx / BLOCK_SIZE;
        
    }
}



pub trait SetGettable {
    fn set(&mut self, idx: usize, val: u8);
    fn get(&self, idx: usize) -> u8;
    fn hash(&self, offset: usize, size: usize) -> Vec<u8>;
    fn size(&self) -> usize;
}

pub struct BinaryBlob {
    bytes: Vec<u8>,
    len: usize,
    block_count: usize
}

impl BinaryBlob {
    // TODO: make this polymorphic, e.g. for text file
    // TODO: handle arbitrary data sizes
    fn new(size: usize) -> BinaryBlob {
        BinaryBlob {
            bytes: (0..size).map(|_| { rand::random::<u8>() }).collect(),
            len: size,
            block_count: size/BLOCK_SIZE,
        }
    }

}

impl SetGettable for BinaryBlob {

    fn set(&mut self, idx: usize, val: u8) {
        self.bytes[idx] = val
    }
    fn get(&self, idx: usize) -> u8 {
        self.bytes[idx]
    }

    fn hash(&self, offset: usize, size: usize) -> Vec<u8> {
        let mut hasher = Sha256::new();
        println!("Hashing block: start - {} len = {}", offset, size);
        println!("Overall len is = {}", self.len);
        hasher.update(&self.bytes[offset..(offset+size)]);
        hasher.finalize().to_vec()
    }

    fn size(&self) -> usize {
        self.len
    }
}



fn main() {

    let matches = App::new("MerkleBench")
        .version("0.1")
        .author("Kyle C. Hale <khale@cs.iit.edu>")
        .about("Measures Merkle Tree update latency over binary blobs.")
        .arg(Arg::with_name("block_count")
            .short("c")
            .value_name("BC")
            .help("Sets the number of blocks to use (4K each), must be a power of 2")
            .takes_value(true))
        .get_matches();

    if matches.is_present("version") {
        println!("Version {}", crate_version!());
        abort();
    }

    let block_count : usize = matches
        .value_of("block_count")
        .unwrap_or("128")
        .trim()
        .parse()
        .unwrap();

    if block_count - 1 & block_count != 0 {
        println!("Must use a power of 2");
        abort();
    }

    println!("Using block count: {}", block_count);

    let blob = BinaryBlob::new(BLOCK_SIZE*block_count);
    /*
    for i in 0..BLOCK_SIZE*block_count {
        println!("a[{}] = {}", i, blob.get(i));
    }
    */

    println!("Constructing Merkle Tree");

    let mut mt = MerkleTree::new(blob);

    println!("Merkle tree created");

    loop {

        let mut user_input = String::new();

        println!("Enter new idx:");

        io::stdin()
            .read_line(&mut user_input)
            .expect("Could not read user input.");

        let nidx: usize = match user_input.trim().parse() {
            Ok(num) => num,
            Err(_) => continue,
        };

        // TODO: refactor
        println!("Enter new val:");

        io::stdin()
            .read_line(&mut user_input)
            .expect("Could not read user input.");

        let nval: u8 = match user_input.trim().parse() {
            Ok(num) => num,
            Err(_) => continue,
        };

        // TODO: measure
        mt.set(nidx, nval);

    }
}


