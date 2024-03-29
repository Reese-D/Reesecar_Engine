use winit::{
    event_loop,
    window,
};

pub fn create_window() -> (event_loop::EventLoop<()>, window::Window) {
    let event_loop = event_loop::EventLoop::new().unwrap();
    let window = window::WindowBuilder::new()
	.with_title("Glorious ReeseCar Engine")
	.build(&event_loop)
	.unwrap();

    event_loop.set_control_flow(event_loop::ControlFlow::Poll);

    return (event_loop, window);
}
