use log::{info, trace, warn};

struct Engine
{

}

impl Engine {

    fn new() -> Self {
	info!("creating engine instance");
	Engine::init_vulkan();
	Engine::main_loop();
	Engine::cleanup();

	Self{}
    }
    fn init_vulkan(){

    }

    fn main_loop(){

    }

    fn cleanup(){
	info!("engine killed, cleaning up");
    }
}

fn main() {
    let reese_car_engine = Engine::new();
}
