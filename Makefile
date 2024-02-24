CXX=clang++
CC=clang
MODULES := src 
# look for include files in each of the modules
CXXFLAGS := $(patsubst %,-I%,$(MODULES)) -std=c++20 -Wall -O2 -Itextures/
# lm = libmath (math.h)
LIBS := -L/usr/local/lib/ -lm -lvulkan -lglfw -lglm
# each module will add to this
SRC :=
# include the description for each module
include $(patsubst %,%/module.mk,$(MODULES))

all: main shaders/frag.spv shaders/vert.spv

# determine the object files
OBJ := $(patsubst %.cpp,%.o, $(filter %.cpp,$(SRC))) $(patsubst %.c,%.o, $(filter %.c,$(SRC)))
# link the program
main: $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LIBS) 
# include the C++ include dependencies
include $(OBJ:.o=.d)
# calculate C++ include dependencies
%.d: %.cpp
	./depend.sh 'irname $*.cpp' $(CFLAGS) $*.cpp > $@
%.d: %.c
	./depend_c.sh 'irname $*.c' $(CFLAGS) $*.c > $@

shaders/frag.spv: shaders/frag.glsl
	cd ./shaders; glslc -fshader-stage=fragment frag.glsl -o frag.spv

shaders/vert.spv: shaders/vert.glsl
	cd ./shaders; glslc -fshader-stage=vertex vert.glsl -o vert.spv

clean:
	find ./ -iname "*.o" -exec rm {} \;
	find ./ -iname "*.d" -exec rm {} \;
	find ./ -iname "*.spv" -exec rm {} \;
	rm main
