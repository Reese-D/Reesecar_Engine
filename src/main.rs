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

struct Queues{
    graphics_queue: ash::vk::Queue
}


struct Engine<'b>
{
    instance: ash::Instance,
    entry: ash::Entry,
    window: window::Window,
    event_loop: Option<event_loop::EventLoop<()>>,
    
    //probably don't need this? It seems fine to let this fall out of scope
    //however it's used in an unsafe block which may reference the memory, so lets keep it alive
    //until our app dies just in case.
    _debug_util_create_info: Option<vk::DebugUtilsMessengerCreateInfoEXT<'b>>,
    
    debug_utils: Option<ash::ext::debug_utils::Instance>,
    debug_util_messenger_ext: Option<vk::DebugUtilsMessengerEXT>,
    physical_device: ash::vk::PhysicalDevice,
    logical_device: ash::Device,
    queues: Queues
}

#[derive(Default)]
struct EngineBuilder<'a,'b>
{
    layers: Option<Vec<&'a str>>,
    extensions: Option<Vec<String>>,
    debug_util: Option<vk::DebugUtilsMessengerCreateInfoEXT<'b>>,
    device_filter: Option<fn(&ash::vk::PhysicalDeviceFeatures, &ash::vk::PhysicalDeviceProperties) -> bool>
}

impl<'a,'b> EngineBuilder<'a,'b> {
    pub fn new() -> Self {
	return EngineBuilder{layers: None, extensions: None, debug_util: None, device_filter: None};
    }

    pub fn enable_validation_layers(&mut self, layers: Vec<&'a str>) -> &mut Self {
	self.layers = Some(layers);
	self.debug_util = Some(Engine::setup_debug_messages());
	return self
    }

    pub fn set_device_filter(&mut self, device_filter: fn(&ash::vk::PhysicalDeviceFeatures, &ash::vk::PhysicalDeviceProperties) -> bool) -> &mut Self{
	self.device_filter = Some(device_filter);
	self
    }

    pub fn enable_extensions(&mut self, extensions: Vec<String>) -> &mut Self {
	self.extensions = Some(extensions);
	self
    }

    pub fn build(&mut self) -> Engine {
	Engine::new(&self.layers, &self.extensions, self.debug_util, self.device_filter)
    }
}

impl<'b> Engine<'b> {

    fn builder<'a,'c>() -> EngineBuilder<'a,'c> {
	EngineBuilder::new()
    }

    fn new(layers: &Option<Vec<&str>>, extensions: &Option<Vec<String>>, debug_util_info: Option<vk::DebugUtilsMessengerCreateInfoEXT<'b>>, device_filter: Option<fn(&ash::vk::PhysicalDeviceFeatures, &ash::vk::PhysicalDeviceProperties) -> bool>) -> Self {
	if std::env::var("RUST_LOG").is_err() {
	    std::env::set_var("RUST_LOG", "info")
	}
	pretty_env_logger::init();
	info!("creating engine instance");
	
	let (event_loop, window) = window_utility::create_window();
	let (entry, instance) = Engine::create_instance(layers, extensions);

	let debug_utils: Option<ash::ext::debug_utils::Instance>;
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

	let physical_device = Engine::pick_physical_device(&instance, device_filter);

	let priority = [1.0f32];
	//for now just finid any queue that supports graphics. TODO pick the best queue if multiple exist, extend to use mulitiple queues.
	let (queue_create_info, index) = Engine::get_physical_device_queue_info(&instance, &physical_device, &priority, |queue_family_properties| {
	    queue_family_properties.iter().position(|queue_family| {
		queue_family.queue_flags.contains(ash::vk::QueueFlags::GRAPHICS)
	    }).unwrap() as u32
	});
	
	let logical_device = Engine::get_logical_device(&instance, &physical_device, queue_create_info);
	
	let queues;
	unsafe {
	    queues = Queues {
		graphics_queue: logical_device.get_device_queue(index, 0)
	    };
	}
	
	Self{instance
	     ,entry
	     ,window
	     ,event_loop: Some(event_loop)
	     ,_debug_util_create_info: debug_util_info
	     ,debug_utils
	     ,debug_util_messenger_ext: debug_util_messenger
	     ,physical_device,
	     logical_device,
	     queues}
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
	let properties;
	unsafe {
	    properties = entry.enumerate_instance_layer_properties();
	}
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

    fn create_debug_util(entry: &ash::Entry, instance: &ash::Instance, debug_create_info: &vk::DebugUtilsMessengerCreateInfoEXT ) -> (ash::ext::debug_utils::Instance, vk::DebugUtilsMessengerEXT) {
	let debug_util = ash::ext::debug_utils::Instance::new(entry, instance);
	unsafe {
	    let debug_util_messenger = debug_util.create_debug_utils_messenger(debug_create_info, None).expect("unable to create debug utils messenger");
	    return (debug_util, debug_util_messenger);
	}
    }

    fn get_physical_device_queue_info<'prio>(instance: &ash::Instance, physical_device: &ash::vk::PhysicalDevice, priority: &'prio[f32], filter: fn(Vec<ash::vk::QueueFamilyProperties>) -> u32) -> (ash::vk::DeviceQueueCreateInfo<'prio> , u32){
	let index: u32;
	unsafe {
	    index = filter(instance.get_physical_device_queue_family_properties(*physical_device));
	}

	//TODO support multiple queues with varying priority
	let result = ash::vk::DeviceQueueCreateInfo::default()
	    .queue_family_index(index)
	    .queue_priorities(priority);

	(result, index)
    }

    fn get_logical_device(instance: &ash::Instance, physical_device: &ash::vk::PhysicalDevice, queue_create_info: ash::vk::DeviceQueueCreateInfo) -> ash::Device {

	//TODO add support to fill out device features
	let device_features = ash::vk::PhysicalDeviceFeatures::default();

	let queue_create_infos = [queue_create_info];
	let device_create_info = ash::vk::DeviceCreateInfo::default()
	    .queue_create_infos(&queue_create_infos)
	    .enabled_features(&device_features);

	unsafe {
	    return instance.create_device(*physical_device, &device_create_info, None).expect("Unable to create vulkan logical device");
	}

    }
    //TODO: consider failing if multiple devices match instead of just returning the first, or adding future support to utilize multiple GPU's
    fn pick_physical_device(instance: &ash::Instance, device_filter: Option<fn(&ash::vk::PhysicalDeviceFeatures, &ash::vk::PhysicalDeviceProperties) -> bool>) -> ash::vk::PhysicalDevice{
	unsafe {
	    let devices = instance.enumerate_physical_devices().expect("Couldn't enumerate physical devices on instance");
	    match device_filter {
		Some(filter) => {
		    for device in devices {
			let features = instance.get_physical_device_features(device);
			let properties = instance.get_physical_device_properties(device);
			if filter(&features, &properties){
			    debug!("Chosen physical device: {0:#?}", properties);
			    return device;
			}
		    };
		},
		None => {
		    debug!("Chosen physical device: {0:#?}",instance.get_physical_device_properties(devices[0])); 
		    return devices[0];
		}
	    }
	}

	panic!("No suitable vulkan physical device could be found that met the minimum necessary requirements");
    }
    
    fn create_instance(layers: &Option<Vec<&str>>, extensions: &Option<Vec<String>>)
		       -> (ash::Entry, ash::Instance) {
	let entry = ash::Entry::linked();
	//TODO pull in app_name as parameter or something
	let app_name = std::ffi::CString::new("Example").expect("Couldn't create application name");
	let engine_name = std::ffi::CString::new("ReeseCar Engine").expect("Couldn't create engine name");
	
	let app_info = vk::ApplicationInfo::default()
	    .application_version(vk::make_api_version(0,1,0,0))
	    .application_name(&app_name)
	    .engine_name(&engine_name)
	    .api_version(vk::make_api_version(0,1,0,0));


	let mut create_info = vk::InstanceCreateInfo::default()
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
		create_info = create_info.enabled_extension_names(&extension_name_ptr);
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
		    create_info = create_info.enabled_layer_names(&layer_name_ptrs);
		} else {
		    warn!("Validation layers were supplied, but the machine doesn't support all of them, validation layers will not be enabled");
		}
	    },
	    None => info!("Validation layers were not provided")
	}

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

    pub fn setup_debug_messages() -> vk::DebugUtilsMessengerCreateInfoEXT<'b> {
	let message_severity = vk::DebugUtilsMessageSeverityFlagsEXT::ERROR
	    | vk::DebugUtilsMessageSeverityFlagsEXT::INFO
	    | vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE
	    | vk::DebugUtilsMessageSeverityFlagsEXT::WARNING;
	
	let message_type = vk::DebugUtilsMessageTypeFlagsEXT::PERFORMANCE
	    | vk::DebugUtilsMessageTypeFlagsEXT::GENERAL
	    | vk::DebugUtilsMessageTypeFlagsEXT::VALIDATION;

	vk::DebugUtilsMessengerCreateInfoEXT::default()
	    .message_severity(message_severity)
	    .message_type(message_type)
	    .pfn_user_callback(Some(Engine::validation_layer_debug_callback))
    }
}

impl<'b> Drop for Engine<'b> {
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
	    self.logical_device.destroy_device(None);
	    self.instance.destroy_instance(None);
	}
    }
}


//Example program
fn main() {

    //Note, you can set the rust_log environment variable programmatically here if you want
    //EX: std::env::set_var("RUST_LOG", "info")
    //by default engine will set this to info if no environment variable is specified
    
    let layers: Vec<&str> = vec!["VK_LAYER_KHRONOS_validation"];
    let mut engine_builder = Engine::builder();
    let mut reese_car_engine = engine_builder
        .enable_validation_layers(layers)
        .enable_extensions(vec![String::from("VK_EXT_debug_utils")])
        .set_device_filter(|features, properties| {
	    info!("shader: {0}, device type: {1:#?}", features.geometry_shader, properties.device_type);
	    return properties.device_type == ash::vk::PhysicalDeviceType::DISCRETE_GPU
		&& features.geometry_shader == vk::TRUE;
	})
        .build();

    info!("-----------VALIDATION LAYERS-----------");
    reese_car_engine.print_validation_layers();
    info!("---------------------------------------");
    
    reese_car_engine.run();
}





