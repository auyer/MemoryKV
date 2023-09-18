use std::net::SocketAddr;
use tracing::Level;
use tracing_subscriber::FmtSubscriber;

use kv::configuration;
use kv::metrics;
use kv::routes;
use kv::shutdown;

#[tokio::main]
async fn main() {
    let config = configuration::read_configuration();

    let mut level = Level::INFO;
    if config.debug {
        level = Level::DEBUG;
    }
    if config.trace {
        level = Level::TRACE;
    }
    let subscriber = FmtSubscriber::builder().with_max_level(level).finish();
    tracing::subscriber::set_global_default(subscriber).expect("setting default subscriber failed");

    tokio::join!(
        server(config.port),
        metrics::start_metrics_server(config.metrics_port)
    );
}

async fn server(port: u16) {
    let app = routes::build_app();

    // Run our app with axum
    let addr = SocketAddr::from(([0, 0, 0, 0], port));
    tracing::info!("listening on {}", addr);

    axum::Server::bind(&addr)
        .serve(app)
        .with_graceful_shutdown(shutdown::shutdown_signal())
        .await
        .expect("server failed");
}
