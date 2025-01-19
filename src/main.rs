extern crate pretty_env_logger;
#[macro_use]
extern crate log;

use winit::{
    dpi::LogicalSize,
    event, event_loop,
    raw_window_handle::{HasDisplayHandle, HasWindowHandle},
    window,
};

use ash::{
    self,
    vk::{self},
};

mod utility;
use utility::window_utility;

struct Queues {
    underlying_queues: Vec<ash::vk::Queue>,
    graphics_queue_index: u32,
    presentation_queue_index: u32,
}

struct Engine<'b> {
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
    queues: Queues,
    surface: vk::SurfaceKHR,
    surface_loader: ash::khr::surface::Instance,
    khr_swapchain: ash::vk::SwapchainKHR,
    swapchain_device: ash::khr::swapchain::Device,
    image_views: Vec<ash::vk::ImageView>
}

#[derive(Default)]
struct EngineBuilder<'b> {
    layers: Option<Vec<String>>,
    instance_extensions: Option<Vec<String>>,
    instance_flags: Option<vk::InstanceCreateFlags>,
    device_extensions: Option<Vec<String>>,
    debug_util: Option<vk::DebugUtilsMessengerCreateInfoEXT<'b>>,
    device_filter: Option<
        fn(
            &ash::vk::PhysicalDeviceFeatures,
            &ash::vk::PhysicalDeviceProperties,
            &Vec<ash::vk::ExtensionProperties>,
        ) -> bool,
    >,
    filter: Option<
        fn(
            vk::SurfaceCapabilitiesKHR,
            Vec<vk::SurfaceFormatKHR>,
            Vec<vk::PresentModeKHR>,
        ) -> (
            ash::vk::PresentModeKHR,
            ash::vk::SurfaceFormatKHR,
            ash::vk::Extent2D,
            vk::ImageUsageFlags,
        ),
    >,
    alter_swapchain_create_info:
        Option<fn(vk::SwapchainCreateInfoKHR) -> vk::SwapchainCreateInfoKHR>,
    width: u32,
    height: u32,
}

impl<'b> EngineBuilder<'b> {
    pub fn new() -> Self {
        return EngineBuilder {
            layers: None,
            instance_extensions: None,
            instance_flags: None,
            device_extensions: None,
            debug_util: None,
            device_filter: None,
            filter: None,
            alter_swapchain_create_info: None,
            width: 800,
            height: 600,
        };
    }

    pub fn enable_validation_layers(&mut self, layers: Vec<String>) -> &mut Self {
        self.layers = Some(layers);
        self.debug_util = Some(Engine::setup_debug_messages());
        return self;
    }

    pub fn set_device_filter(
        &mut self,
        device_filter: fn(
            &ash::vk::PhysicalDeviceFeatures,
            &ash::vk::PhysicalDeviceProperties,
            &Vec<ash::vk::ExtensionProperties>,
        ) -> bool,
    ) -> &mut Self {
        self.device_filter = Some(device_filter);
        self
    }

    pub fn enable_instance_flags(&mut self, flags: vk::InstanceCreateFlags) -> &mut Self {
        self.instance_flags = Some(flags);
        self
    }

    pub fn enable_instance_extensions(&mut self, extensions: Vec<String>) -> &mut Self {
        self.instance_extensions = Some(extensions);
        self
    }

    pub fn enable_device_extensions(&mut self, extensions: Vec<String>) -> &mut Self {
        self.device_extensions = Some(extensions);
        self
    }

    fn get_logical_defaults_for_swapchain_filter() -> fn(
        vk::SurfaceCapabilitiesKHR,
        Vec<vk::SurfaceFormatKHR>,
        Vec<vk::PresentModeKHR>,
    ) -> (
        ash::vk::PresentModeKHR,
        ash::vk::SurfaceFormatKHR,
        ash::vk::Extent2D,
        vk::ImageUsageFlags,
    ) {
        |surface_capabilities, surface_formats, present_modes| {
            let available_formats = surface_formats
                .iter()
                .filter(|surface_format| {
                    surface_format.format == ash::vk::Format::B8G8R8A8_SRGB
                        && surface_format.color_space
                            == ash::vk::ColorSpaceKHR::EXTENDED_SRGB_NONLINEAR_EXT
                })
                .collect::<Vec<_>>();

            let format = available_formats.first().unwrap();

            let available_present_modes = present_modes
                .iter()
                .filter(|present_mode| **present_mode == ash::vk::PresentModeKHR::MAILBOX)
                .collect::<Vec<_>>();

            let present_mode = available_present_modes.first().unwrap();

            let mut extent = surface_capabilities.current_extent;
            if extent.width == u32::MAX {
                let min = surface_capabilities.min_image_extent;
                let max = surface_capabilities.max_image_extent;
                let width = 800.clamp(min.width, max.width);
                let height = 600.clamp(min.height, max.height);
                extent = vk::Extent2D { width, height };
            }

            (
                **present_mode,
                **format,
                extent,
                vk::ImageUsageFlags::COLOR_ATTACHMENT,
            )
        }
    }

    pub fn set_window_dimensions(&mut self, width: u32, height: u32) -> &mut Self {
        self.width = width;
        self.height = height;
        self
    }

    pub fn build(&mut self) -> Engine {
        let swapchain_filter;
        match self.filter {
            Some(filter) => {
                swapchain_filter = filter;
            }
            None => swapchain_filter = Self::get_logical_defaults_for_swapchain_filter(),
        }
        Engine::new(
            &self.layers,
            &self.instance_extensions,
            self.instance_flags,
            &self.device_extensions,
            self.debug_util,
            self.device_filter,
            swapchain_filter,
            self.alter_swapchain_create_info,
            LogicalSize::new(self.width, self.height),
        )
    }

    pub fn set_swapchain_filter(
        &mut self,
        filter: fn(
            vk::SurfaceCapabilitiesKHR,
            Vec<vk::SurfaceFormatKHR>,
            Vec<vk::PresentModeKHR>,
        ) -> (
            ash::vk::PresentModeKHR,
            ash::vk::SurfaceFormatKHR,
            ash::vk::Extent2D,
            vk::ImageUsageFlags,
        ),
        alter_swapchain_create_info: Option<
            fn(vk::SwapchainCreateInfoKHR) -> vk::SwapchainCreateInfoKHR,
        >,
    ) -> &mut Self {
        self.filter = Some(filter);
        self.alter_swapchain_create_info = alter_swapchain_create_info;
        return self;
    }
}

impl<'b> Engine<'b> {
    fn builder<'c>() -> EngineBuilder<'c> {
        EngineBuilder::new()
    }

    fn new(
        layers: &Option<Vec<String>>,
        instance_extensions: &Option<Vec<String>>,
        instance_flags: Option<vk::InstanceCreateFlags>,
        device_extensions: &Option<Vec<String>>,
        debug_util_info: Option<vk::DebugUtilsMessengerCreateInfoEXT<'b>>,
        device_filter: Option<
            fn(
                &ash::vk::PhysicalDeviceFeatures,
                &ash::vk::PhysicalDeviceProperties,
                &Vec<ash::vk::ExtensionProperties>,
            ) -> bool,
        >,
        filter: fn(
            vk::SurfaceCapabilitiesKHR,
            Vec<vk::SurfaceFormatKHR>,
            Vec<vk::PresentModeKHR>,
        ) -> (
            ash::vk::PresentModeKHR,
            ash::vk::SurfaceFormatKHR,
            ash::vk::Extent2D,
            vk::ImageUsageFlags,
        ),
        alter_swapchain_create_info: Option<
            fn(vk::SwapchainCreateInfoKHR) -> vk::SwapchainCreateInfoKHR,
        >,
        window_dimensions: LogicalSize<u32>,
    ) -> Self {
        if std::env::var("RUST_LOG").is_err() {
            std::env::set_var("RUST_LOG", "info")
        }
        pretty_env_logger::init();
        info!("creating engine instance");

        let (event_loop, window) = window_utility::create_window(window_dimensions);

        let full_instance_extensions = Engine::configure_extensions(&window, instance_extensions);

        let (entry, instance) =
            Engine::create_instance(layers, &full_instance_extensions, instance_flags);
        let (surface, surface_loader) = Engine::get_surface(&entry, &instance, &window);

        let debug_utils: Option<ash::ext::debug_utils::Instance>;
        let debug_util_messenger: Option<vk::DebugUtilsMessengerEXT>;
        match debug_util_info {
            Some(debug_util_create_info) => {
                let (x, y) = Engine::create_debug_util(&entry, &instance, &debug_util_create_info);
                debug_utils = Some(x);
                debug_util_messenger = Some(y);
            }
            None => {
                debug_utils = None;
                debug_util_messenger = None;
            }
        }

        let physical_device = Engine::pick_physical_device(&instance, device_filter);

        let priority = [1.0f32];
        //for now just find any queue that supports graphics & presentation. TODO pick the best queue if multiple exist, extend to use mulitiple queues.
        let queue_infos_and_indexes = Engine::create_physical_device_queue_info(
            &instance,
            &physical_device,
            &priority,
            |queue_family_properties| {
                let mut graphics_indexes = vec![];
                let mut presentation_indexes = vec![];
                for (index, queue_family) in queue_family_properties.iter().enumerate() {
                    if queue_family
                        .queue_flags
                        .contains(ash::vk::QueueFlags::GRAPHICS)
                    {
                        graphics_indexes.push(index);
                    }
                    unsafe {
                        let supports_surface = surface_loader
                            .get_physical_device_surface_support(
                                physical_device,
                                index as u32,
                                surface,
                            )
                            .expect("couldn't check physical device support for given surface");
                        if supports_surface {
                            presentation_indexes.push(index);
                        }
                    }
                }
                if graphics_indexes.len() == 0 || presentation_indexes.len() == 0 {
                    return None;
                }
                if graphics_indexes[0] == presentation_indexes[0] {
                    debug!("presentation and graphics will share the same queue");
                    return Some(vec![graphics_indexes[0] as u32]);
                }
                //couldn't find a queue that supports graphics & presentation, so we need to create two queues
                debug!("Presentation and graphics will use two separate queues");
                return Some(vec![
                    graphics_indexes[0] as u32,
                    presentation_indexes[0] as u32,
                ]);
            },
        )
        .expect("Couldn't find a queue that supports graphics");

        let logical_device = Engine::get_logical_device(
            &instance,
            &physical_device,
            queue_infos_and_indexes.iter().map(|item| item.0).collect(),
            &device_extensions,
        );

        let underlying_queues = queue_infos_and_indexes
            .iter()
            .map(|item| {
                unsafe {
                    logical_device.get_device_queue(item.1, 0) //note the 0, this just means we want the first queue from this family
                }
            })
            .collect();

        let queues = Queues {
            underlying_queues,
            //not the same index as above, those indexes indicated index within the queue family, these indexes indicate the index within the underyling_queue vec
            graphics_queue_index: 0,
            presentation_queue_index: (queue_infos_and_indexes.len() - 1) as u32, //could be the same index as graphics_queue, but otherwise is the next and final item
        };

        let swapchain_info = Engine::create_swapchain_info(
            &physical_device,
            &surface_loader,
            surface,
            filter,
            alter_swapchain_create_info,
        );

	let swapchain_device = ash::khr::swapchain::Device::new(&instance, &logical_device);
	let khr_swapchain;
	let swapchain_images : Vec<vk::Image>;
	unsafe {
	    let khr_swapchain_result = swapchain_device.create_swapchain(&swapchain_info, None);

	    match khr_swapchain_result {
		Ok(x) => {
		    khr_swapchain = x;
		}, Err(x) => {
		    //TODO try and handle this error instead of panicking?
		    panic!("Couldn't make a khr_swapchain with the given swapchain info. Error was {x:?}");
		}
	    }

	    let swapchain_image_result = swapchain_device.get_swapchain_images(khr_swapchain);

	    match swapchain_image_result {
		Ok(x) => {
		    swapchain_images = x;
		}, Err(x) => {
		    //TODO try and handle this error instead of panicking?
		    panic!("Couldn't make a khr_swapchain images Error was {x:?}");
		}
	    }
	}

	let mut mut_image_views = vec![];
	for i in swapchain_images {
	    let image_view_info = ash::vk::ImageViewCreateInfo{
		image: i,
		view_type: ash::vk::ImageViewType::TYPE_2D,
		components: vk::ComponentMapping {
		    r: ash::vk::ComponentSwizzle::IDENTITY,
		    g: ash::vk::ComponentSwizzle::IDENTITY,
		    b: ash::vk::ComponentSwizzle::IDENTITY,
		    a: ash::vk::ComponentSwizzle::IDENTITY
		},
		subresource_range: vk::ImageSubresourceRange
		{
		    aspect_mask: ash::vk::ImageAspectFlags::COLOR,
		    base_mip_level: 0,
		    level_count: 1,
		    base_array_layer: 0,
		    layer_count: 1
		},
		format: ash::vk::Format::B8G8R8A8_SRGB, //TODO this needs to match the swapchain_create_info call's image_format value.
		..Default::default()
	    };
	    
	    let result;
	    unsafe {
		result = logical_device.create_image_view(&image_view_info, None);
	    }
	    match result {
		Ok(x) => {
		    mut_image_views.push(x);
		}, Err(x) => {
		    //TODO try and handle this error instead of panicking?
		    panic!("Couldn't make a khr_swapchain images Error was {x:?}");
		}
	    }
	}

	let image_views = mut_image_views; //move to immutable
	//TODO need to destroy all the image_views

        Self {
            instance,
            entry,
            window,
            event_loop: Some(event_loop),
            _debug_util_create_info: debug_util_info,
            debug_utils,
            debug_util_messenger_ext: debug_util_messenger,
            physical_device,
            logical_device,
            queues,
            surface,
            surface_loader,
	    khr_swapchain,
	    swapchain_device,
	    image_views
        }
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
                        event::Event::WindowEvent {
                            event: event::WindowEvent::CloseRequested,
                            ..
                        } => {
                            info!("Close button pressed on window, killing window");
                            elwt.exit();
                        }
                        event::Event::AboutToWait => {
                            //request redraw only if the window actually needs to update
                            //If we *always* want to redraw then just do it here and don't trigger request_redraw()
                            self.window.request_redraw();
                        }
                        event::Event::WindowEvent {
                            event: event::WindowEvent::RedrawRequested,
                            ..
                        } => {
                            //Redraw application (for applications that don't redraw every time)
                        }
                        _ => (),
                    };
                });
            }
            None => panic!("Could not run, event loop has no value!"),
        }

        //our eventloop is now out of scope and gets destroyed. Engine.event_loop is now None
    }

    fn configure_extensions(
        window: &window::Window,
        extensions: &Option<Vec<String>>,
    ) -> Vec<String> {
        let extension_names = ash_window::enumerate_required_extensions(
            window
                .display_handle()
                .expect("Could not get display handle from window")
                .as_raw(),
        )
        .unwrap()
        .to_vec();

        let mut final_extension_list: Vec<String> = vec![];
        for item in extension_names {
            unsafe {
                let result = std::ffi::CStr::from_ptr(item).to_str().unwrap().to_string();
                final_extension_list.push(result);
            }
        }

        match extensions {
            Some(extension) => {
                for item in extension {
                    final_extension_list.push(item.clone());
                }
            }
            None => {}
        }

        //seems to be included by ash_window automatically, but in case it's system dependent add if missing
        let debug_util_name = ash::ext::debug_utils::NAME.to_str().unwrap().to_string();
        if !final_extension_list.contains(&debug_util_name) {
            final_extension_list.push(debug_util_name);
        }

        debug!("Extensions:");
        for extension in &final_extension_list {
            debug!("    {}", extension);
        }

        return final_extension_list;
    }

    fn create_swapchain_info<'swapchain>(
        physical_device: &vk::PhysicalDevice,
        surface_loader: &ash::khr::surface::Instance,
        surface: vk::SurfaceKHR,
        filter: fn(
            vk::SurfaceCapabilitiesKHR,
            Vec<vk::SurfaceFormatKHR>,
            Vec<vk::PresentModeKHR>,
        ) -> (
            ash::vk::PresentModeKHR,
            ash::vk::SurfaceFormatKHR,
            ash::vk::Extent2D,
            vk::ImageUsageFlags,
        ),
        alter_swapchain_create_info: Option<
            fn(vk::SwapchainCreateInfoKHR) -> vk::SwapchainCreateInfoKHR,
        >,
    ) -> vk::SwapchainCreateInfoKHR<'swapchain> {
        let capabilities;
        let formats;
        let present_modes;
        unsafe {
            capabilities = surface_loader
                .get_physical_device_surface_capabilities(*physical_device, surface)
                .expect("Unable to get physical device surface capabilities");
            formats = surface_loader
                .get_physical_device_surface_formats(*physical_device, surface)
                .expect("Unable to get physical device surface formats");
            present_modes = surface_loader
                .get_physical_device_surface_present_modes(*physical_device, surface)
                .expect("Unable to get physical device surface presentation modes");
        }
        let (present_mode, surface_format, extent, image_usage_flags) =
            filter(capabilities, formats, present_modes);


        let swapchain_create_info = vk::SwapchainCreateInfoKHR::default()
            .image_extent(extent)
            .present_mode(present_mode)
            .image_format(surface_format.format)
            .surface(surface)
            .image_color_space(surface_format.color_space)
            .image_usage(image_usage_flags)
            .image_sharing_mode(vk::SharingMode::EXCLUSIVE) //Requires explicit ownership transfer
            .pre_transform(capabilities.current_transform) 
            .composite_alpha(ash::vk::CompositeAlphaFlagsKHR::OPAQUE)
            .image_array_layers(1) //TODO check if this is what we actually want
            .min_image_count(capabilities.min_image_count)
            .clipped(true);

        match alter_swapchain_create_info {
            Some(filter) => {
                return filter(swapchain_create_info);
            }
            None => {
                return swapchain_create_info;
            }
        }
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
        } else {
            properties.unwrap().iter().for_each(|layer| {
                let layer_name = unsafe { std::ffi::CStr::from_ptr(layer.layer_name.as_ptr()) };
                f(&layer_name
                    .to_str()
                    .expect("Couldn't retreive layer name ptr"));
            });
        }
    }

    pub fn check_validation_layers(entry: &ash::Entry, layers: &Vec<String>) -> bool {
        let mut count = 0;
        Engine::operate_over_validation_layers(entry, &mut |layer_name| {
            let result: bool = layers.iter().any(|name| name == &layer_name);
            if result {
                count += 1;
            }
        });

        count >= layers.len()
    }

    fn get_surface(
        entry: &ash::Entry,
        instance: &ash::Instance,
        window: &window::Window,
    ) -> (vk::SurfaceKHR, ash::khr::surface::Instance) {
        let surface;
        unsafe {
            surface = ash_window::create_surface(
                &entry,
                &instance,
                window
                    .display_handle()
                    .expect("could not retrieve window's display handle")
                    .as_raw(),
                window
                    .window_handle()
                    .expect("could not retrieve window's window handle")
                    .as_raw(),
                None,
            )
            .expect("Unable to create ash_window surface");
        }
        let surface_loader = ash::khr::surface::Instance::new(&entry, &instance);
        return (surface, surface_loader);
    }

    fn create_debug_util(
        entry: &ash::Entry,
        instance: &ash::Instance,
        debug_create_info: &vk::DebugUtilsMessengerCreateInfoEXT,
    ) -> (ash::ext::debug_utils::Instance, vk::DebugUtilsMessengerEXT) {
        let debug_util = ash::ext::debug_utils::Instance::new(entry, instance);
        unsafe {
            let debug_util_messenger = debug_util
                .create_debug_utils_messenger(debug_create_info, None)
                .expect("unable to create debug utils messenger");
            return (debug_util, debug_util_messenger);
        }
    }

    fn create_physical_device_queue_info<'prio>(
        instance: &ash::Instance,
        physical_device: &ash::vk::PhysicalDevice,
        priority: &'prio [f32],
        filter: impl Fn(Vec<ash::vk::QueueFamilyProperties>) -> Option<Vec<u32>>,
    ) -> Option<Vec<(ash::vk::DeviceQueueCreateInfo<'prio>, u32)>> {
        let indexes: Vec<u32>;
        unsafe {
            let result =
                filter(instance.get_physical_device_queue_family_properties(*physical_device));

            if result.is_none() {
                return None;
            }
            indexes = result.unwrap();
        }

        let mut results = vec![];
        for index in indexes {
            results.push((
                ash::vk::DeviceQueueCreateInfo::default()
                    .queue_family_index(index)
                    .queue_priorities(priority),
                index,
            ));
        }
        return Some(results);
    }

    fn get_logical_device(
        instance: &ash::Instance,
        physical_device: &ash::vk::PhysicalDevice,
        queue_create_info: Vec<ash::vk::DeviceQueueCreateInfo>,
        extensions: &Option<Vec<String>>,
    ) -> ash::Device {
        //TODO add support to fill out device features
        let device_features = ash::vk::PhysicalDeviceFeatures::default();

        let mut device_create_info = ash::vk::DeviceCreateInfo::default()
            .queue_create_infos(&queue_create_info)
            .enabled_features(&device_features);

        let extension_name;
        let extension_name_ptr;

        match extensions {
            Some(extension) => {
                extension_name = extension
                    .iter()
                    .map(|item| std::ffi::CString::new(item.as_bytes()).expect(""))
                    .collect::<Vec<_>>();
                extension_name_ptr = extension_name
                    .iter()
                    .map(|item| item.as_ptr())
                    .collect::<Vec<_>>();

                device_create_info =
                    device_create_info.enabled_extension_names(&extension_name_ptr);
            }
            None => {}
        }

        unsafe {
            return instance
                .create_device(*physical_device, &device_create_info, None)
                .expect("Unable to create vulkan logical device");
        }
    }
    //TODO: consider failing if multiple devices match instead of just returning the first, or adding future support to utilize multiple GPU's
    fn pick_physical_device(
        instance: &ash::Instance,
        device_filter: Option<
            fn(
                &ash::vk::PhysicalDeviceFeatures,
                &ash::vk::PhysicalDeviceProperties,
                &Vec<ash::vk::ExtensionProperties>,
            ) -> bool,
        >,
    ) -> ash::vk::PhysicalDevice {
        unsafe {
            let devices = instance
                .enumerate_physical_devices()
                .expect("Couldn't enumerate physical devices on instance");
            match device_filter {
                Some(filter) => {
                    for device in devices {
                        let features = instance.get_physical_device_features(device);
                        let properties = instance.get_physical_device_properties(device);
                        let extensions = instance
                            .enumerate_device_extension_properties(device)
                            .expect("Unable to enumerate deevice's extension properties");
                        if filter(&features, &properties, &extensions) {
                            debug!("Chosen physical device: {0:#?}", properties);
                            return device;
                        }
                    }
                }
                None => {
                    debug!(
                        "Chosen physical device: {0:#?}",
                        instance.get_physical_device_properties(devices[0])
                    );
                    return devices[0];
                }
            }
        }

        panic!("No suitable vulkan physical device could be found that met the minimum necessary requirements");
    }

    fn create_instance(
        layers: &Option<Vec<String>>,
        extensions: &Vec<String>,
        flags: Option<vk::InstanceCreateFlags>,
    ) -> (ash::Entry, ash::Instance) {
        let entry = ash::Entry::linked();
        //TODO pull in app_name as parameter or something
        let app_name = std::ffi::CString::new("Example").expect("Couldn't create application name");
        let engine_name =
            std::ffi::CString::new("ReeseCar Engine").expect("Couldn't create engine name");

        let app_info = vk::ApplicationInfo::default()
            .application_version(vk::make_api_version(0, 1, 0, 0))
            .application_name(&app_name)
            .engine_name(&engine_name)
            .api_version(vk::make_api_version(0, 1, 0, 0));

        let mut create_info = vk::InstanceCreateInfo::default().application_info(&app_info);

        match flags {
            Some(x) => {
                create_info = create_info.flags(x);
            }
            None => {}
        }

        //declaring these outside the match blocks extends the lifetime to mimic the scope of create_info
        let extension_name;
        let extension_name_ptr;
        let layer_names;
        let layer_name_ptrs;

        extension_name = extensions
            .iter()
            .map(|item| std::ffi::CString::new(item.as_bytes()).expect(""))
            .collect::<Vec<_>>();
        extension_name_ptr = extension_name
            .iter()
            .map(|item| item.as_ptr())
            .collect::<Vec<_>>();
        create_info = create_info.enabled_extension_names(&extension_name_ptr);


        match layers {
            Some(x) => {
                layer_names = x
                    .iter()
                    .map(|name| std::ffi::CString::new(name.clone()).expect("couldn't get layer name ptr"))
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
            }
            None => info!("Validation layers were not provided"),
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
        _p_user_data: *mut std::ffi::c_void,
    ) -> vk::Bool32 {
        let message = format!(
            "{0:#?}: {1:#?}",
            message_types,
            std::ffi::CStr::from_ptr((*p_callback_data).p_message)
        );
        match message_severity {
            vk::DebugUtilsMessageSeverityFlagsEXT::VERBOSE => {
                debug!("{}", message);
            }
            vk::DebugUtilsMessageSeverityFlagsEXT::INFO => {
                info!("{}", message);
            }
            vk::DebugUtilsMessageSeverityFlagsEXT::WARNING => {
                warn!("{}", message);
            }
            vk::DebugUtilsMessageSeverityFlagsEXT::ERROR => {
                error!("{} ", message);
            }
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
                Some(x) => match &self.debug_utils {
                    Some(y) => {
                        y.destroy_debug_utils_messenger(x, None);
                    }
                    None => {}
                },
                None => {}
            };

	    for image_view in &self.image_views {
		self.logical_device.destroy_image_view(*image_view, None);
	    }
	    self.swapchain_device.destroy_swapchain(self.khr_swapchain, None);
            self.surface_loader.destroy_surface(self.surface, None);
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
    std::env::set_var("RUST_LOG", "debug");

    let layers: Vec<String> = vec![String::from("VK_LAYER_KHRONOS_validation")];
    let mut engine_builder = Engine::builder();

    let mut reese_car_engine = engine_builder
        .enable_validation_layers(layers)
        .enable_instance_extensions(vec![
            String::from("VK_EXT_debug_utils"), //for MacOS
            String::from("VK_KHR_portability_enumeration"),
            String::from("VK_KHR_get_physical_device_properties2"),
        ])
        // .set_device_filter(|features, properties, extensions| {
        //     info!(
        //         "shader: {0}, device type: {1:#?}",
        //         features.geometry_shader, properties.device_type
        //     );
        //     debug!(
        //         "Extensions available for device: {0:#?}",
        //         extensions
        //             .iter()
        //             .fold(String::from(""), |accumulator, element| {
        //                 return accumulator
        //                     + " "
        //                     + element.extension_name_as_c_str().unwrap().to_str().unwrap();
        //             })
        //     );
        //     unsafe {
        //         return //properties.device_type == //ash::vk::PhysicalDeviceType::DISCRETE_GPU
        //             //&&
        // 	    features.geometry_shader == vk::TRUE
        //             && extensions
        //                 .iter()
        //                 .find(|x| {
        //                     std::ffi::CStr::from_ptr(x.extension_name.as_ptr())
        //                         == ash::vk::KHR_SWAPCHAIN_NAME
        //                 })
        //                 .is_some();
        //     }
        // })
        .enable_instance_flags(vk::InstanceCreateFlags::ENUMERATE_PORTABILITY_KHR)
        .enable_device_extensions(vec![String::from("VK_KHR_portability_subset"), String::from("VK_KHR_swapchain")]) //portability is for MacOS
        .set_swapchain_filter(
            |surface_capabilities, surface_formats, present_modes| {
                let available_formats = surface_formats
                    .iter()
                    .filter(|surface_format| {
                        debug!("surface format: {:#?}", surface_format);
                        return surface_format.format == ash::vk::Format::B8G8R8A8_SRGB
                            && surface_format.color_space
                                == ash::vk::ColorSpaceKHR::SRGB_NONLINEAR;
                    })
                    .collect::<Vec<_>>();

                let format = available_formats.first().unwrap();

                let available_present_modes = present_modes
                    .iter()
                    .filter(|present_mode| {
                        debug!("present_mode: {:#?}", present_mode);
                        return **present_mode == ash::vk::PresentModeKHR::IMMEDIATE;
                    })
                    .collect::<Vec<_>>();

                let present_mode = available_present_modes.first().unwrap();

                // let min = surface_capabilities.min_image_extent;
                // let max = surface_capabilities.max_image_extent;
                // let width = 800.clamp(min.width, max.width);
                // let height = 600.clamp(min.height, max.height);
                // let extent = vk::Extent2D { width, height };

                let extent = surface_capabilities.current_extent;
                (
                    **present_mode,
                    **format,
                    extent,
                    vk::ImageUsageFlags::COLOR_ATTACHMENT,
                )
            },
            None,
        )
        .build();

    info!("-----------VALIDATION LAYERS-----------");
    reese_car_engine.print_validation_layers();
    info!("---------------------------------------");

    reese_car_engine.run();
}
