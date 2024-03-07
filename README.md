This is a placeholder readme for the reese-car engine

This engine is in construction and should not be used.

# Setup instructions
You'll need to install the following libraries in order to compile:
glfw
vulkan-sdk
glm

You'll also need these programs to compile
clang (alternatively update the Makefile and switch to your preferred compiler)

And if you're developing the engine you'll also want *shaderc*, which allows compilation of the glsl shaders to spirv

You can also use Nix-os to build this, see the flake and lock files, even if you're not using nix (I ususally don't) the flake file helps describe the required packages.

Suggested packages:
bear -- If you need to update the compile_commands.json for lsp, I use this with emacs.
emacs -- development environment, you can use an editor of your choice however

# Build instructions
bear -- make
    or alternatively
make

### to clean up files
make clean
