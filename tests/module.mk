SRC_FILES=test

TEST_SRC += $(addsuffix .cpp, $(addprefix tests/, $(SRC_FILES)))
