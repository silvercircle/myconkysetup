
mod context;

fn main() {
    println!("Hello, world!");

    let ctx: &context::Context = context::get_instance();
    println!("Name is and I have been used {} times", ctx._use_count);

    let ctx1 = context::get_instance();
    println!("Name is and I have been used {} times", ctx1._use_count);
    ctx1.inc_use_count();
    println!("Name is and I have been used {} times", ctx1._use_count);

    println!("Config dir: {:?}", ctx1._cfg._config_dir);
    println!("Data dir:   {:?}", ctx1._cfg._data_dir);
    println!("Log File:   {:?}", ctx1._cfg._log_file_name);
    ctx1.cleanup();
}
