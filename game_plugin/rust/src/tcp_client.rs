use std::{
    io,
    net::SocketAddr,
    sync::mpsc::{self, Receiver},
    thread::{self, JoinHandle},
    time::Duration,
};

use futures_util::{SinkExt, StreamExt};
use tokio::{
    net::TcpStream,
    runtime::Builder,
    sync::{mpsc as tokio_mpsc, watch},
};
use tokio_util::codec::{Framed, LinesCodec};

const HELLO_MESSAGE: &str = r#"{"game":"Saints Row 2","protocol":3,"type":"hello"}"#;
const MAX_LINE_LENGTH: usize = 64 * 1024;
const OUTGOING_QUEUE_CAPACITY: usize = 128;
const RECONNECT_DELAY: Duration = Duration::from_secs(2);

enum NetworkEvent {
    Connected,
    Disconnected,
    Line(String),
}

pub(crate) struct TcpClient {
    outgoing: Option<tokio_mpsc::Sender<String>>,
    events: Receiver<NetworkEvent>,
    shutdown: watch::Sender<bool>,
    worker: Option<JoinHandle<()>>,
    connected: bool,
}

impl TcpClient {
    pub(crate) fn new(port: u16) -> io::Result<Self> {
        Self::new_with_reconnect_delay(port, RECONNECT_DELAY)
    }

    fn new_with_reconnect_delay(port: u16, reconnect_delay: Duration) -> io::Result<Self> {
        let (outgoing_tx, outgoing_rx) = tokio_mpsc::channel(OUTGOING_QUEUE_CAPACITY);
        let (event_tx, event_rx) = mpsc::channel();
        let (shutdown_tx, shutdown_rx) = watch::channel(false);

        let runtime = Builder::new_current_thread()
            .enable_io()
            .enable_time()
            .build()?;

        let worker = thread::Builder::new()
            .name("sr2ap-network".into())
            .spawn(move || {
                runtime.block_on(network_worker(
                    port,
                    reconnect_delay,
                    outgoing_rx,
                    event_tx,
                    shutdown_rx,
                ));
            })?;

        Ok(Self {
            outgoing: Some(outgoing_tx),
            events: event_rx,
            shutdown: shutdown_tx,
            worker: Some(worker),
            connected: false,
        })
    }

    pub(crate) fn send_line(&self, line: &str) -> io::Result<()> {
        let Some(outgoing) = &self.outgoing else {
            return Err(io::Error::new(
                io::ErrorKind::BrokenPipe,
                "network worker stopped",
            ));
        };

        outgoing
            .try_send(line.to_owned())
            .map_err(|error| match error {
                tokio_mpsc::error::TrySendError::Full(_) => {
                    io::Error::new(io::ErrorKind::WouldBlock, "outgoing network queue is full")
                }
                tokio_mpsc::error::TrySendError::Closed(_) => {
                    io::Error::new(io::ErrorKind::BrokenPipe, "network worker stopped")
                }
            })
    }

    pub(crate) fn poll(&mut self, mut handle_line: impl FnMut(&str)) {
        while let Ok(event) = self.events.try_recv() {
            match event {
                NetworkEvent::Connected => self.connected = true,
                NetworkEvent::Disconnected => self.connected = false,
                NetworkEvent::Line(line) => handle_line(&line),
            }
        }
    }

    pub(crate) fn is_connected(&self) -> bool {
        self.connected
    }
}

impl Drop for TcpClient {
    fn drop(&mut self) {
        let _ = self.shutdown.send(true);
        self.outgoing.take();

        if let Some(worker) = self.worker.take() {
            if worker.join().is_err() {
                log::error!(target: "Network", "Network worker panicked during shutdown");
            }
        }
    }
}

async fn network_worker(
    port: u16,
    reconnect_delay: Duration,
    mut outgoing: tokio_mpsc::Receiver<String>,
    events: mpsc::Sender<NetworkEvent>,
    mut shutdown: watch::Receiver<bool>,
) {
    let address = SocketAddr::from(([127, 0, 0, 1], port));

    loop {
        let connection = tokio::select! {
            _ = shutdown.changed() => return,
            result = TcpStream::connect(address) => result,
        };

        let stream = match connection {
            Ok(stream) => stream,
            Err(error) => {
                log::debug!(
                    target: "Network",
                    "Connection attempt to {address} failed: {error}; retrying"
                );

                if !wait_before_reconnect(&mut shutdown, &mut outgoing, reconnect_delay).await {
                    return;
                }
                continue;
            }
        };

        let mut framed = Framed::new(stream, LinesCodec::new_with_max_length(MAX_LINE_LENGTH));
        let hello_result = tokio::select! {
            _ = shutdown.changed() => return,
            result = framed.send(HELLO_MESSAGE.to_owned()) => result,
        };

        if let Err(error) = hello_result {
            log::debug!(
                target: "Network",
                "Could not send hello message to {address}: {error}; retrying"
            );

            if !wait_before_reconnect(&mut shutdown, &mut outgoing, reconnect_delay).await {
                return;
            }
            continue;
        }

        log::info!(target: "Network", "Connected to AP client on {address}");
        if events.send(NetworkEvent::Connected).is_err() {
            return;
        }

        let (mut writer, mut reader) = framed.split();
        let disconnect_reason = loop {
            tokio::select! {
                _ = shutdown.changed() => return,

                outgoing_line = outgoing.recv() => {
                    let Some(line) = outgoing_line else {
                        return;
                    };

                    let send_result = tokio::select! {
                        _ = shutdown.changed() => return,
                        result = writer.send(line) => result,
                    };

                    if let Err(error) = send_result {
                        break error.to_string();
                    }
                }

                incoming_line = reader.next() => {
                    match incoming_line {
                        Some(Ok(line)) => {
                            log::debug!(target: "Network", "Received: {line}");
                            if events.send(NetworkEvent::Line(line)).is_err() {
                                return;
                            }
                        }
                        Some(Err(error)) => break error.to_string(),
                        None => break "AP client closed the connection".to_owned(),
                    }
                }
            }
        };

        log::warn!(
            target: "Network",
            "Disconnected from AP client: {disconnect_reason}; retrying"
        );
        if events.send(NetworkEvent::Disconnected).is_err() {
            return;
        }

        if !wait_before_reconnect(&mut shutdown, &mut outgoing, reconnect_delay).await {
            return;
        }
    }
}

async fn wait_before_reconnect(
    shutdown: &mut watch::Receiver<bool>,
    outgoing: &mut tokio_mpsc::Receiver<String>,
    reconnect_delay: Duration,
) -> bool {
    let delay = tokio::time::sleep(reconnect_delay);
    tokio::pin!(delay);

    loop {
        tokio::select! {
            biased;
            _ = shutdown.changed() => return false,
            _ = &mut delay => return true,
            message = outgoing.recv() => {
                if message.is_none() {
                    return false;
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use std::{
        io::{BufRead, BufReader, Write},
        net::{TcpListener, TcpStream},
        thread,
        time::{Duration, Instant},
    };

    use super::*;

    const TEST_TIMEOUT: Duration = Duration::from_secs(3);

    fn configure_test_stream(stream: &TcpStream) {
        stream
            .set_read_timeout(Some(TEST_TIMEOUT))
            .expect("set test stream read timeout");
        stream
            .set_write_timeout(Some(TEST_TIMEOUT))
            .expect("set test stream write timeout");
    }

    #[test]
    fn client_should_exchange_newline_delimited_messages() {
        let _logger_guard = crate::logger::TEST_LOCK
            .lock()
            .expect("lock logger during TCP test");
        let listener = TcpListener::bind(("127.0.0.1", 0)).expect("bind test server");
        let port = listener.local_addr().expect("read test address").port();
        let server = thread::spawn(move || {
            let (mut stream, _) = listener.accept().expect("accept client");
            configure_test_stream(&stream);
            let mut reader = BufReader::new(stream.try_clone().expect("clone test stream"));

            let mut hello = String::new();
            reader.read_line(&mut hello).expect("read hello message");

            writeln!(stream, r#"{{"type":"test_message"}}"#).expect("send test message");
            stream.flush().expect("flush test message");

            let mut outgoing = String::new();
            reader
                .read_line(&mut outgoing)
                .expect("read outgoing message");

            (hello, outgoing)
        });

        let mut client = TcpClient::new(port).expect("create TCP client");
        let deadline = Instant::now() + TEST_TIMEOUT;
        let mut incoming = Vec::new();

        while Instant::now() < deadline && (!client.is_connected() || incoming.is_empty()) {
            client.poll(|line| incoming.push(line.to_owned()));
            thread::sleep(Duration::from_millis(5));
        }

        client
            .send_line(r#"{"type":"outgoing"}"#)
            .expect("queue outgoing message");

        let (hello, outgoing) = server.join().expect("join test server");
        assert_eq!(
            (
                client.is_connected(),
                hello.trim_end(),
                outgoing.trim_end(),
                incoming.as_slice(),
            ),
            (
                true,
                HELLO_MESSAGE,
                r#"{"type":"outgoing"}"#,
                [r#"{"type":"test_message"}"#.to_owned()].as_slice(),
            )
        );
    }

    #[test]
    fn client_should_reconnect_after_server_closes_connection() {
        let _logger_guard = crate::logger::TEST_LOCK
            .lock()
            .expect("lock logger during TCP test");
        let listener = TcpListener::bind(("127.0.0.1", 0)).expect("bind test server");
        let port = listener.local_addr().expect("read test address").port();
        let (release_server, wait_for_release) = mpsc::sync_channel(0);
        let server = thread::spawn(move || {
            for connection_index in 0..2 {
                let (mut stream, _) = listener.accept().expect("accept client");
                configure_test_stream(&stream);
                let mut reader = BufReader::new(stream.try_clone().expect("clone test stream"));
                let mut hello = String::new();
                reader.read_line(&mut hello).expect("read hello message");
                assert_eq!(hello.trim_end(), HELLO_MESSAGE);

                if connection_index == 1 {
                    writeln!(stream, r#"{{"type":"after_reconnect"}}"#)
                        .expect("send message after reconnect");
                    stream.flush().expect("flush message after reconnect");
                    wait_for_release
                        .recv_timeout(TEST_TIMEOUT)
                        .expect("wait for reconnect assertion");
                }
            }
        });

        let mut client = TcpClient::new_with_reconnect_delay(port, Duration::from_millis(10))
            .expect("create TCP client");
        let deadline = Instant::now() + TEST_TIMEOUT;
        let mut incoming = Vec::new();

        while Instant::now() < deadline && incoming.is_empty() {
            client.poll(|line| incoming.push(line.to_owned()));
            thread::sleep(Duration::from_millis(5));
        }

        client.poll(|line| incoming.push(line.to_owned()));
        let actual = (client.is_connected(), incoming);
        release_server.send(()).expect("release test server");
        server.join().expect("join test server");

        assert_eq!(
            actual,
            (true, vec![r#"{"type":"after_reconnect"}"#.to_owned()])
        );
    }
}
