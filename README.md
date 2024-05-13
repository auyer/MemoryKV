# MemoryKV

I like to learn thing by doing them.
This is a in memory Key Value store in Rust. It also has a Write Ahead Log served with Websockets.

# About this project

What I wanted to create:

- In memory database with a hashmap structure
- Using concurrent patterns
- In the Rust programming language
- With a live feed of changes (it felt like a cool demo for my personal site)

I started implementing an http web server using Axum. I did this because I am used to working with http and rest interfaces. I might implement my own connection protocol, but this will be fine for now.
Since I was using Axum I also added a `/metrics` endpoint for Prometheus.

The "database" part I created with a simple HashMap, and protected it behind a RWMutex.

The "WAL/live feed" part was implemented internally using a Broadcast channel. The http part exposes this as a websocket.

All of this is shared between threads (or tokio tasks) with Arc (Atomic Atomically Reference Counted pointers).

A sample of the WAL received by a WebSocket client:

```prolog has the right colors
23:42 23.17 Heartbeat from 127.0.0.1:36460
23:42 14.02 Read { key: pre:fix:for_key_value:perfect}
23:42 03.70 List
23:42 03.17 Heartbeat from 127.0.0.1:53374
23:41 29.96 Read { key: pre:demo:key}
23:41 23.17 Heartbeat from 127.0.0.1:36460
23:41 17.34 ListPrefix { prefix: pre}
23:41 05.19 Insert { key: pre:demo:key, value: {"source": "CloudFlare Worker", "type": "WASM"} }
23:41 03.19 Heartbeat from 127.0.0.1:53374
23:41 03.18 Connected to ws://localhost:8080/wal
23:41 03.17 Connecting to ws://localhost:8080/wal
```

The live demo is available in my personal website: https://rcpassos.me/projects/kv
And the source for that is here:
https://github.com/auyer/auyer.github.io/blob/main/src/lib/services.js

# SDKs
I am also creating SDKs for this project using different languages.
Since the server uses REST with HTTP for the database interface, this is the main part that needs to be implemented in the SDKs.
The Live Feed is implemented using WebSockets, and is a less important feature.


| Feature/SDK    	| C 	| Rust 	
|----------------	|---	|------	
| Rest Endpoints 	| x 	|      	
| Live Feed (WS) 	|   	|      	
