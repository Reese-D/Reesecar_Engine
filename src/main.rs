extern crate pretty_env_logger;
#[macro_use] extern crate log;

use winit::{
    event,
    event_loop,
    window,
};

struct Engine
{

}

impl Engine {

    fn new() -> Self {
	if std::env::var("RUST_LOG").is_err() {
	    std::env::set_var("RUST_LOG", "info")
	}
	pretty_env_logger::init();
	info!("creating engine instance");
	
	let (event_loop, window) = Engine::create_window();
	Engine::init_vulkan();
	Engine::main_loop();
	Engine::cleanup();

	//Will likely be moved into main loop in the future
	let _ = event_loop.run(move |window_event, elwt| {
	    match window_event {
		event::Event::WindowEvent { event: event::WindowEvent::CloseRequested, ..} => {
		    info!("Close button pressed on window, killing window");
		    elwt.exit();
		},
		event::Event::AboutToWait => {
		    //request redraw only if the window actually needs to update
		    //If we *always* want to redraw then just do it here and don't trigger request_redraw()
		    window.request_redraw();
		},
		event::Event::WindowEvent { event: event::WindowEvent::RedrawRequested, ..} => {
		    //Redraw application (for applications that don't redraw every time)
		},
		_ => ()
	    };
	});
	
	Self{}
    }
    
    fn create_window() -> (event_loop::EventLoop<()>, window::Window) {
	let event_loop = event_loop::EventLoop::new().unwrap();
	let window = window::WindowBuilder::new()
	    .with_title("Glorious ReeseCar Engine")
	    .build(&event_loop)
	    .unwrap();

	event_loop.set_control_flow(event_loop::ControlFlow::Poll);

	return (event_loop, window);
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
