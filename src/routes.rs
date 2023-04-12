use axum::{
    error_handling::HandleErrorLayer,
    extract::DefaultBodyLimit,
    handler::Handler,
    middleware,
    routing::{delete, get, IntoMakeService},
    Router,
};
use std::{sync::Arc, time::Duration};
use tower::ServiceBuilder;
use tower_http::{
    compression::CompressionLayer, limit::RequestBodyLimitLayer, trace::TraceLayer,
    validate_request::ValidateRequestHeaderLayer,
};

use crate::db::{KVStore, KVStoreConn};
use crate::handlers::{
    delete_all_keys, handle_error, kv_get, kv_set, list_keys, list_keys_with_prefix, remove_key,
};
use crate::metrics;

pub fn build_app() -> IntoMakeService<Router> {
    // move out
    let shared_state = KVStore::new();

    // Build our application by composing routes
    let app = Router::new()
        .route(
            "/:key",
            // Add compression to `kv_get`
            get(kv_get.layer(CompressionLayer::new()))
                // But don't compress `kv_set`
                .post_service(
                    kv_set
                        .layer((
                            DefaultBodyLimit::disable(),
                            RequestBodyLimitLayer::new(1024 * 5_000 /* ~5mb */),
                        ))
                        .with_state(Arc::clone(&shared_state)),
                ),
        )
        .route("/keys", get(list_keys))
        .route("/keys/:prefix", get(list_keys_with_prefix))
        // Nest our admin routes under `/admin`
        .nest("/admin", admin_routes())
        // Add middleware to all routes
        .layer(
            ServiceBuilder::new()
                // Handle errors from middleware
                .layer(HandleErrorLayer::new(handle_error))
                .load_shed()
                .concurrency_limit(1024)
                .timeout(Duration::from_secs(10))
                .layer(TraceLayer::new_for_http()),
        )
        .layer(metrics::create_tracing_layer())
        .layer(middleware::from_fn(metrics::track_metrics))
        .with_state(Arc::clone(&shared_state));
    return app.into_make_service();
}

fn admin_routes() -> Router<KVStoreConn> {
    Router::new()
        .route("/keys", delete(delete_all_keys))
        .route("/key/:key", delete(remove_key))
        // Require bearer auth for all admin routes
        .layer(ValidateRequestHeaderLayer::bearer("secret-token"))
}
