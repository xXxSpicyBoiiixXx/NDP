mod server;

use std::error::Error;
use log::{trace, debug, info, warn, error};
use clap::Clap;
use bolt::nodes::try_listen_on;
use crate::server::ServerContext;

#[derive(Clap)]
#[clap(version = "0.1", author = "Josh Bowden <jbowden@hawk.iit.edu>, Md Ali <mali54@hawk.iit.edu>")]
struct Opts {
    /// An alternative port to listen on
    #[clap(short, long, default_value = "9090")]
    port: u16,
}

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    pretty_env_logger::init();

    let opts: Opts = Opts::parse();
    let listen_port_preferred = opts.port;

    let mut listener = try_listen_on("127.0.0.1", listen_port_preferred).await?;
    let listen_address = listener.local_addr().unwrap();
    if listen_address.port() != listen_port_preferred {
        warn!("Unable to listen on specified port {} since its already in use. Using port {} instead.", listen_port_preferred, listen_address.port())
    }
    info!("listening on {}", &listen_address);

    let ctx = ServerContext::with_capacity(1024, 128);
    server::run(&mut listener, ctx).await?;
    Ok(())
}
