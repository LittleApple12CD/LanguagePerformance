use {
    std::env,
    std::io,
    winresource::WindowsResource,
};

fn main() -> io::Result<()> {
    if env::var_os("CARGO_CFG_WINDOWS").is_some() {
        WindowsResource::new()
            .set_icon("icon.ico")
            .compile()?;
    }
    Ok(())
}