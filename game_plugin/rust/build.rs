use std::{env, path::PathBuf};

fn main() {
    let crate_dir =
        PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").expect("Cargo manifest directory"));

    let output = crate_dir.join("../include/sr2ap/sr2ap_rust.h");

    println!("cargo::rerun-if-changed=src");
    println!("cargo::rerun-if-changed=cbindgen.toml");

    cbindgen::Builder::new()
        .with_crate(&crate_dir)
        .with_config(
            cbindgen::Config::from_file(crate_dir.join("cbindgen.toml"))
                .expect("valid cbindgen configuration"),
        )
        .generate()
        .expect("generate C header")
        .write_to_file(output);
}
