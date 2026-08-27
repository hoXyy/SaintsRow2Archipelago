use crate::tcp_client::TcpClient;

#[cxx::bridge(namespace = "sr2ap::rust")]
pub(crate) mod bridge {
    struct TcpPollResult {
        connected: bool,
        lines: Vec<String>,
    }

    extern "Rust" {
        type TcpClient;

        fn tcp_client_new(port: u16) -> Result<Box<TcpClient>>;
        fn tcp_client_send_line(client: &TcpClient, line: &[u8]) -> Result<()>;
        fn tcp_client_poll(client: &mut TcpClient) -> TcpPollResult;
    }
}

#[derive(Debug, thiserror::Error)]
enum TcpClientFfiError {
    #[error(transparent)]
    Io(#[from] std::io::Error),

    #[error("outgoing message is not valid UTF-8")]
    Utf8(#[from] std::str::Utf8Error),
}

fn tcp_client_new(port: u16) -> Result<Box<TcpClient>, std::io::Error> {
    TcpClient::new(port).map(Box::new)
}

fn tcp_client_send_line(client: &TcpClient, line: &[u8]) -> Result<(), TcpClientFfiError> {
    let line = std::str::from_utf8(line)?;
    client.send_line(line)?;
    Ok(())
}

fn tcp_client_poll(client: &mut TcpClient) -> bridge::TcpPollResult {
    let mut lines = Vec::new();

    client.poll(|line| {
        lines.push(line.to_owned());
    });

    bridge::TcpPollResult {
        connected: client.is_connected(),
        lines,
    }
}
