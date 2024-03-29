extern crate pretty_env_logger;
#[macro_use] extern crate log;

use winit::{
    event,
    event_loop,
    window,
};

use ash;

struct Engine
{
    instance: ash::Instance
}

impl Engine {

    fn new() -> Self {
	if std::env::var("RUST_LOG").is_err() {
	    std::env::set_var("RUST_LOG", "info")
	}
	pretty_env_logger::init();
	info!("creating engine instance");
	
	let (event_loop, window) = Engine::create_window();
	let instance = Engine::create_instance();

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
	
	Self{instance}
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

    fn create_instance() -> ash::Instance {
	let entry = ash::Entry::linked();
	//TODO pull in app_name as parameter or something
	let app_name = std::ffi::CString::new("Example").expect("Couldn't create application name");
	let engine_name = std::ffi::CString::new("ReeseCar Engine").expect("Couldn't create engine name");
	
	let app_info = ash::vk::ApplicationInfo::builder()
	    .application_version(ash::vk::make_api_version(0,1,0,0))
	    .application_name(&app_name)
	    .engine_name(&engine_name)
	    .build();

	let create_info = ash::vk::InstanceCreateInfo {
	    p_application_info: &app_info,
	    ..Default::default()
	};
	unsafe {
	    let instance = entry
		.create_instance(&create_info, None)
		.expect("Instance creation error");
	    return instance;
	}
    }

    fn main_loop(){

    }
}

impl Drop for Engine {
    fn drop(&mut self) {
	unsafe {
	    info!("Engine destroyed, cleaning up");
	    self.instance.destroy_instance(None);
	}
    }
}

fn main() {
    let reese_car_engine = Engine::new();
}
