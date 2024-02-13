SRC_FILES= instance main queue surface validation device swapchain

SRC += $(addsuffix .cpp, $(addprefix src/, $(SRC_FILES)))
