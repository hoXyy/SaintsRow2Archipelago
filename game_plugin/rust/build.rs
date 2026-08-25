fn main() {
    cxx_build::CFG.include_prefix = "sr2ap";

    cxx_build::bridge("src/ffi/logger_ffi.rs")
        .std("c++17")
        .compile("sr2ap-cxxbridge");

    println!("cargo::rerun-if-changed=src/ffi.rs");
}
