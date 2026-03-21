CXX := clang++
ALL_WARN_FLAGS := -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-declarations -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused 
INCLUDE := -Iexternal -Iexternal/KTX-Software/lib/include -Iexternal/KTX-Software/external/dfdutils/ -Iexternal/tinyobjloader -I/usr/local/include/ -I/usr/include/ -Iexternal/vulkan-sdk/include/
## Note: libSDL3.so was manually installed, -lSDL3 should be used on most platforms and installed through package manager
LIBDIR := -L/usr/local/lib/ -L/usr/lib/ -lvulkan -Lexternal/vulkan-sdk/lib/ -Lexternal/KTX-Software/build/ 
LIB := -lSDL3 -lslang -lktx -lslang-compiler
## These tell the compiled binary where to look for the .so files that were linked, typically these are in /usr/lib/ or /usr/local/lib
LIB_PATHS := -Wl,-rpath='./external/KTX-Software/build/',-rpath='./external/vulkan-sdk/lib/'
CXXFLAGS := --std=c++26

main:
	$(CXX) src/main.cpp -o main $(CXXFLAGS) $(ALL_WARN_FLAGS) $(INCLUDE) $(LIBDIR) $(LIB) $(LIB_PATHS) 
clean:
	rm main
format:
	find src -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -i
