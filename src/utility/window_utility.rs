use winit::{
    event_loop, window, dpi::LogicalSize
};

pub fn create_window(window_dimensions: LogicalSize<u32>) -> (event_loop::EventLoop<()>, window::Window) {
    let event_loop = event_loop::EventLoop::new();

    match event_loop {
	Ok(x) => {
	    x.set_control_flow(event_loop::ControlFlow::Poll);
	    let window = window::WindowBuilder::new()
		.with_title("Glorious ReeseCar Engine")
		.with_inner_size(window_dimensions)
		.build(&x)
		.unwrap();
	    return (x, window);
	},
	_ => panic!("Couldn't create an event loop with winit"),
    }
}
