extern crate pretty_env_logger;
#[macro_use] extern crate log;

use winit::{
    event_loop,
    window,
    event
};

use ash;

mod utility;
use utility::window_utility;

struct Engine
{
    instance: ash::Instance,
    entry: ash::Entry,
    window: window::Window,
    event_loop: Option<event_loop::EventLoop<()>>
}

#[derive(Default)]
struct EngineBuilder<'a>
{
    layers: Option<Vec<&'a str>>
}

impl<'a> EngineBuilder<'a> {
    pub fn new() -> Self {
	return EngineBuilder{layers: None};
    }

    pub fn enable_validation_layers(&mut self, layers: Vec<&'a str>) -> &mut Self {
	self.layers = Some(layers);
	return self
    }

    pub fn build(&self) -> Engine {
	Engine::new(&self.layers)
    }
}

impl Engine {

    fn builder<'a>() -> EngineBuilder<'a> {
	EngineBuilder::new()
    }

    fn new(layers: &Option<Vec<&str>>) -> Self {
	if std::env::var("RUST_LOG").is_err() {
	    std::env::set_var("RUST_LOG", "info")
	}
	pretty_env_logger::init();
	info!("creating engine instance");
	
	let (event_loop, window) = window_utility::create_window();
	let (entry, instance) = Engine::create_instance(layers);
	
	Self{instance, entry, window, event_loop: Some(event_loop)}
    }

    pub fn run(&mut self) {
	//.run takes ownership, however since our struct implements drop we aren't allowed
	//to move event_loop out of the struct. To work around this we swap the event loop
	//with another one, and then use the correct version to run
	let mut swapped: Option<event_loop::EventLoop<()>> = None;
	std::mem::swap(&mut self.event_loop, &mut swapped);
	match swapped {
	    Some(x) => {
		let _ = x.run(move |window_event, elwt| {
		    match window_event {
			event::Event::WindowEvent { event: event::WindowEvent::CloseRequested, ..} => {
			    info!("Close button pressed on window, killing window");
			    elwt.exit();
			},
			event::Event::AboutToWait => {
			    //request redraw only if the window actually needs to update
			    //If we *always* want to redraw then just do it here and don't trigger request_redraw()
			    self.window.request_redraw();
			},
			event::Event::WindowEvent { event: event::WindowEvent::RedrawRequested, ..} => {
			    //Redraw application (for applications that don't redraw every time)
			},
			_ => ()
		    };
		});

	    },
	    None => panic!("Could not run, event loop has no value!")
	}

	//our eventloop is now out of scope and gets destroyed. Engine.event_loop is now None
    }

    pub fn print_validation_layers(&self) {
	Engine::operate_over_validation_layers(&self.entry, &mut |name| {
	    info!("{name}");
	});
    }

    pub fn operate_over_validation_layers(entry: &ash::Entry, f: &mut dyn FnMut(&str)) {
	let properties = entry.enumerate_instance_layer_properties();
	
	if properties.is_err() {
	    warn!("Cannot enable validation layers, enumerate_instance_layer_properties failed");
	    return;
	}
	else {
	    properties
		.unwrap()
		.iter()
		.for_each(|layer| {
		    let layer_name = unsafe { std::ffi::CStr::from_ptr(layer.layer_name.as_ptr()) };
		    f(&layer_name.to_str().expect("Couldn't retreive layer name ptr"));
		});
	}
    }

    pub fn check_validation_layers(entry: &ash::Entry, layers: &Vec<&str>) -> bool {
	let mut count = 0;
	Engine::operate_over_validation_layers(entry, &mut |layer_name| {
	    let result: bool = layers
		.iter()
		.any(|name| name == &layer_name);
	    if result {
		count += 1;
	    }
	});

	count >= layers.len()
    }
    
    fn create_instance(layers: &Option<Vec<&str>>) -> (ash::Entry, ash::Instance) {
	let entry = ash::Entry::linked();
	//TODO pull in app_name as parameter or something
	let app_name = std::ffi::CString::new("Example").expect("Couldn't create application name");
	let engine_name = std::ffi::CString::new("ReeseCar Engine").expect("Couldn't create engine name");
	
	let app_info = ash::vk::ApplicationInfo::builder()
	    .application_version(ash::vk::make_api_version(0,1,0,0))
	    .application_name(&app_name)
	    .engine_name(&engine_name)
	    .api_version(ash::vk::make_api_version(0,1,0,0))
	    .build();

	let mut create_info_builder = ash::vk::InstanceCreateInfo::builder()
	    .application_info(&app_info);

	//pre-declaring these extends the lifetime to match the scope of create_info
	let layer_names;
	let layer_name_ptrs;
	
	//enable validation if it's not None
	match layers {
	    Some(x) => {
		layer_names = x
		    .iter()
		    .map(|name| std::ffi::CString::new(*name).expect("couldn't get layer name ptr"))
		    .collect::<Vec<_>>();
		
		layer_name_ptrs = layer_names
		    .iter()
		    .map(|name| name.as_ptr())
		    .collect::<Vec<_>>();
		
		let result: bool = Engine::check_validation_layers(&entry, x);
		
		if result {
		    create_info_builder = create_info_builder.enabled_layer_names(&layer_name_ptrs);
		} else {
		    warn!("Validation layers were supplied, but the machine doesn't support all of them, validation layers will not be enabled");
		}
	    },
	    None => info!("Validation layers were not provided")
	}

	let create_info = create_info_builder.build();

	unsafe {
	    let instance = entry
		.create_instance(&create_info, None)
		.expect("Instance creation error");
	    return (entry, instance);
	}
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
    let layers: Vec<&str> = vec!["VK_LAYER_KHRONOS_validation"];    
    let mut reese_car_engine = Engine::builder()
        .enable_validation_layers(layers)
        .build();
    info!("-----------VALIDATION LAYERS-----------");
    reese_car_engine.print_validation_layers();
    info!("---------------------------------------");
    reese_car_engine.run();
}

