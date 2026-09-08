#[derive(Debug, Default, PartialEq, Eq)]
pub(super) enum Context {
    #[default]
    Waiting,
    Provisional,
    AwaitingCursor,
    ActiveRevision,
}

#[derive(Default)]
pub(super) struct Delivery {
    pub context: Context,
    pub checksum: Option<u32>,
    pub next_index: u64,
}

impl Delivery {
    pub fn ready(&self) -> bool {
        matches!(self.context, Context::Provisional | Context::ActiveRevision)
    }

    pub fn load(&mut self, checksum: u32) {
        self.checksum = Some(checksum);
        self.next_index = 0;
        self.context = Context::AwaitingCursor;
    }
}
