fn main() {
    cxx_build::CFG.include_prefix = "sr2ap";

    cxx_build::bridges([
        "src/ffi/logger_ffi.rs",
        "src/ffi/config_ffi.rs",
        "src/ffi/protocol_ffi.rs",
        "src/ffi/revision_journal_ffi.rs",
        "src/ffi/tcp_client_ffi.rs",
        "src/ffi/session_ffi.rs",
    ])
    .std("c++17")
    .cpp(true)
    .compile("sr2ap-cxxbridge");

    println!("cargo::rerun-if-changed=src/ffi/logger_ffi.rs");
    println!("cargo::rerun-if-changed=src/ffi/config_ffi.rs");
    println!("cargo::rerun-if-changed=src/ffi/protocol_ffi.rs");
    println!("cargo::rerun-if-changed=src/ffi/revision_journal_ffi.rs");
    println!("cargo::rerun-if-changed=src/ffi/tcp_client_ffi.rs");
    println!("cargo::rerun-if-changed=src/ffi/session_ffi.rs");
}
