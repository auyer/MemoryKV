use axum::extract::connect_info::IntoMakeServiceWithConnectInfo;
use axum::{
    error_handling::HandleErrorLayer,
    extract::DefaultBodyLimit,
    http::{header, Method},
    middleware,
    routing::get,
    Router,
};
use std::{net::SocketAddr, time::Duration};
use tower::ServiceBuilder;
use tower_http::cors::{Any, CorsLayer};
use tower_http::{
    compression::CompressionLayer,
    limit::RequestBodyLimitLayer,
    trace::TraceLayer,
    // validate_request::ValidateRequestHeaderLayer,
};

use crate::db::KVStore;
use crate::handlers;
use crate::metrics;

pub fn build_app() -> IntoMakeServiceWithConnectInfo<Router, std::net::SocketAddr> {
    let cors = CorsLayer::new()
        // allow `GET`, `PUT` and `DELETE` when accessing the resource
        .allow_methods([Method::GET, Method::PUT, Method::DELETE])
        .allow_headers(vec![header::ACCEPT, header::CONTENT_TYPE])
        // allow requests from any origin
        .allow_origin(Any);
    // move out
    let shared_state = KVStore::new();

    // Build our application by composing routes
    let app = Router::new()
        .route("/wal", get(handlers::ws_handler))
        .route("/ping", get(handlers::ping))
        .route(
            "/:key",
            get(handlers::kv_get)
                .put(handlers::kv_set)
                .delete(handlers::remove_key),
        )
        .route(
            "/keys",
            get(handlers::list_keys).delete(handlers::remove_all_keys),
        )
        .route(
            "/keys/:prefix",
            get(handlers::list_keys_with_prefix).delete(handlers::remove_prefix),
        )
        .layer((
            DefaultBodyLimit::disable(),
            RequestBodyLimitLayer::new(1024 * 5_000 /* ~5mb */),
        ))
        .layer(CompressionLayer::new())
        .layer(cors)
        .layer(
            ServiceBuilder::new()
                // Handle errors from middleware
                .layer(HandleErrorLayer::new(handlers::handle_error))
                .load_shed()
                .concurrency_limit(1024)
                .timeout(Duration::from_secs(10))
                .layer(TraceLayer::new_for_http()),
        )
        .layer(metrics::create_tracing_layer())
        .layer(middleware::from_fn(metrics::track_metrics))
        .with_state(shared_state.clone());
    return app.into_make_service_with_connect_info::<SocketAddr>();
}
