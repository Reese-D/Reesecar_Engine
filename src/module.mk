SRC_FILES= instance main queue surface validation device

SRC += $(addsuffix .cpp, $(addprefix src/, $(SRC_FILES)))
