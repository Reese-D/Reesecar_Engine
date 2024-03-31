extern crate pretty_env_logger;
#[macro_use] extern crate log;

use winit::{
    event_loop,
    window,
    event
};

use ash::{
    self,
    vk::{self}
};

mod utility;
use utility::window_utility;

struct Engine
{
    instance: ash::Instance,
    entry: ash::Entry,
    window: window::Window,
    event_loop: Option<event_loop::EventLoop<()>>,
    
    //probably don't need this? It seems fine to let this fall out of scope
    //however it's used in an unsafe block which may reference the memory, so lets keep it alive
    //until our app dies just in case.
    _debug_util_create_info: Option<vk::DebugUtilsMessengerCreateInfoEXT>,
    
    debug_utils: Option<ash::extensions::ext::DebugUtils>,
    debug_util_messenger_ext: Option<vk::DebugUtilsMessengerEXT>
}

#[derive(Default)]
struct EngineBuilder<'a>
{
    layers: Option<Vec<&'a str>>,
    extensions: Option<Vec<String>>,
    debug_util: Option<vk::DebugUtilsMessengerCreateInfoEXT>
}

impl<'a> EngineBuilder<'a> {
    pub fn new() -> Self {
	return EngineBuilder{layers: None, extensions: None, debug_util: None};
    }

    pub fn enable_validation_layers(&mut self, layers: Vec<&'a str>) -> &mut Self {
	self.layers = Some(layers);
	self.debug_util = Some(Engine::setup_debug_messages());
	return self
    }

    pub fn enable_extensions(&mut self, extensions: Vec<String>) -> &mut Self {
	self.extensions = Some(extensions);
	self
    }

    pub fn build(&mut self) -> Engine {
	Engine::new(&self.layers, &self.extensions, self.debug_util)
    }
}

impl Engine {

    fn builder<'a>() -> EngineBuilder<'a> {
	EngineBuilder::new()
    }

    fn new(layers: &Option<Vec<&str>>, extensions: &Option<Vec<String>>, debug_util_info: Option<vk::DebugUtilsMessengerCreateInfoEXT>) -> Self {
	if std::env::var("RUST_LOG").is_err() {
	    std::env::set_var("RUST_LOG", "info")
	}
	pretty_env_logger::init();
	info!("creating engine instance");
	
	let (event_loop, window) = window_utility::create_window();
	let (entry, instance) = Engine::create_instance(layers, extensions);

	let debug_utils: Option<ash::extensions::ext::DebugUtils>;
	let debug_util_messenger: Option<vk::DebugUtilsMessengerEXT>;
	match debug_util_info {
	    Some(debug_util_create_info) => {
		let (x,y) = Engine::create_debug_util(&entry, &instance, &debug_util_create_info);
		debug_utils = Some(x);
		debug_util_messenger = Some(y);
	    }, None => {
		debug_utils = None;
		debug_util_messenger = None;
	    }
	}
	Self{instance
	     ,entry
	     ,window
	     ,event_loop: Some(event_loop)
	     ,_debug_util_create_info: debug_util_info
	     ,debug_utils
	     ,debug_util_messenger_ext: debug_util_messenger}
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

    fn create_debug_util(entry: &ash::Entry, instance: &ash::Instance, debug_create_info: &vk::DebugUtilsMessengerCreateInfoEXT ) -> (ash::extensions::ext::DebugUtils, vk::DebugUtilsMessengerEXT) {
	let debug_util = ash::extensions::ext::DebugUtils::new(entry, instance);
	unsafe {
	    let debug_util_messenger = debug_util.create_debug_utils_messenger(debug_create_info, None).expect("unable to create debug utils messenger");
	    return (debug_util, debug_util_messenger);
	}
    }
    
    fn create_instance(layers: &Option<Vec<&str>>, extensions: &Option<Vec<String>>)
		       -> (ash::Entry, ash::Instance) {
	let entry = ash::Entry::linked();
	//TODO pull in app_name as parameter or something
	let app_name = std::ffi::CString::new("Example").expect("Couldn't create application name");
	let engine_name = std::ffi::CString::new("ReeseCar Engine").expect("Couldn't create engine name");
	
	let app_info = vk::ApplicationInfo::builder()
	    .application_version(vk::make_api_version(0,1,0,0))
	    .application_name(&app_name)
	    .engine_name(&engine_name)
	    .api_version(vk::make_api_version(0,1,0,0))
	    .build();

	let mut create_info_builder = vk::InstanceCreateInfo::builder()
	    .application_info(&app_info);
	
	//declaring these outside the match blocks extends the lifetime to mimic the scope of create_info
	let extension_name;
	let extension_name_ptr;
	let layer_names;
	let layer_name_ptrs;

	match extensions
	{
	    Some(x) => {
		extension_name = x.iter()
		    .map(|item| {
			std::ffi::CString::new(item.as_bytes()).expect("")
		    })
		    .collect::<Vec<_>>();
		extension_name_ptr = extension_name.iter()
		    .map(|item| item.as_ptr())
		    .collect::<Vec<_>>();
		create_info_builder = create_info_builder.enabled_extension_names(&extension_name_ptr);
	    },
	    None => { info!("No extensions provided, none will be enabled"); }
	}

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
		
		let result: bool = Engine::check_validation_layers(&entry, &x);
		
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
    
    pub unsafe extern "system" fn validation_layer_debug_callback(
	message_severity: vk::DebugUtilsMessageSeverityFlagsEXT,
	message_types: vk::DebugUtilsMessageTypeFlagsEXT,
	p_callback_data: *const vk::DebugUtilsMessengerCallbackDataEXT,
	_p_user_data: *mut std::ffi::c_void
    ) -> vk::Bool32 {
	let message = format!("{0:#?}: {1:#?}", message_types, std::ffi::CStr::from_ptr((*p_callback_data).p_message));
	match message_severity {
	    vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE => {
		debug!("{}", message);
	    },
	    vk::DebugUtilsMessageSeverityFlagsEXT::INFO => {
		info!("{}", message);
	    },
	    vk::DebugUtilsMessageSeverityFlagsEXT::WARNING => {
		warn!("{}", message);
	    },
	    vk::DebugUtilsMessageSeverityFlagsEXT::ERROR => {
		error!("{} ",  message);
	    },
	    _ => {
		error!("{}", message);
	    }
	}
	vk::FALSE
    }

    pub fn setup_debug_messages() -> vk::DebugUtilsMessengerCreateInfoEXT {
	let message_severity = vk::DebugUtilsMessageSeverityFlagsEXT::ERROR
	    | vk::DebugUtilsMessageSeverityFlagsEXT::INFO
	    | vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE
	    | vk::DebugUtilsMessageSeverityFlagsEXT::WARNING;
	
	let message_type = vk::DebugUtilsMessageTypeFlagsEXT::PERFORMANCE
	    | vk::DebugUtilsMessageTypeFlagsEXT::GENERAL
	    | vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION;

	vk::DebugUtilsMessengerCreateInfoEXT::builder()
	    .message_severity(message_severity)
	    .message_type(message_type)
	    .pfn_user_callback(Some(Engine::validation_layer_debug_callback))
	    .build()
    }
}

impl Drop for Engine {
    fn drop(&mut self) {
	unsafe {
	    info!("Engine destroyed, cleaning up");
	    match self.debug_util_messenger_ext {
		Some(x) => {
		    match &self.debug_utils {
			Some(y) => {
			    y.destroy_debug_utils_messenger(x, None);
			},
			None => {}
		    }
		},
		None => {}
	    };

	    self.instance.destroy_instance(None);
	}
    }
}

fn main() {
    let layers: Vec<&str> = vec!["VK_LAYER_KHRONOS_validation"];    
    let mut reese_car_engine = Engine::builder()
        .enable_validation_layers(layers)
        .enable_extensions(vec![String::from("VK_EXT_debug_utils")])
        .build();

    info!("-----------VALIDATION LAYERS-----------");
    reese_car_engine.print_validation_layers();
    info!("---------------------------------------");
    
    reese_car_engine.run();
}

