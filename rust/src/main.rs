extern crate log;
extern crate simplelog;

mod context;

fn main() {
    let ctx = context::get_instance();
    ctx.inc_use_count();
    ctx.cleanup();
}
