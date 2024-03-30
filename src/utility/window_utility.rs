use winit::{
    event_loop, window
};

pub fn create_window() -> (event_loop::EventLoop<()>, window::Window) {
    let event_loop = event_loop::EventLoop::new();

    match event_loop {
	Ok(x) => {
	    x.set_control_flow(event_loop::ControlFlow::Poll);
	    let window = window::WindowBuilder::new()
		.with_title("Glorious ReeseCar Engine")
		.build(&x)
		.unwrap();
	    return (x, window);
	},
	_ => panic!("Couldn't create an event loop with winit"),
    }
}
