SRC_FILES= instance main queue surface validation device swapchain graphics_pipeline

SRC += $(addsuffix .cpp, $(addprefix src/, $(SRC_FILES)))
