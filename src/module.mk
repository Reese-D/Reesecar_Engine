SRC_FILES= instance main queue surface validation device swapchain graphics_pipeline render_pass

SRC += $(addsuffix .cpp, $(addprefix src/, $(SRC_FILES)))
