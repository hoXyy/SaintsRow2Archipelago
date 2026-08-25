fn main() {
    cxx_build::CFG.include_prefix = "sr2ap";

    cxx_build::bridge("src/ffi/logger_ffi.rs")
        .std("c++17")
        .cpp(true)
        .compile("sr2ap-cxxbridge");

    println!("cargo::rerun-if-changed=src/ffi/logger_ffi.rs");
}
