This is a placeholder readme for the reese-car engine

This engine is in construction and should not be used.

# Setup instructions
You'll need to install the following libraries in order to compile:
`vulkan-sdk`

and if you're on MacOs you may also need
`molten-vk`

You'll also need these programs to compile
Cargo

And if you're developing the engine you'll also want *shaderc*, which allows compilation of the glsl shaders to spirv

Suggested packages:
emacs -- development environment, you can use an editor of your choice however

# Build instructions
`cargo build`

# Run
`cargo run`

# Debug instructions (lldb, macos)

first install (I actually build from source instead)
`llvm
lld
lldb`

and optionally if building from source you may want to add

`clang
clang-extra-tools`

Rpath doesn't seem to work properly on macos so you'll need to use the following command to point to the linked libraries

Note these exclude the hash at the end of reesecar_engine, you'll need to use whatever the latest hash is
if you're not sure which to use, just run `cargo clean` and build again, there will only be a single hash afterwards.

`install_name_tool -add_rpath /usr/local/lib target/debug/deps/reesecar_engine` 

and run the debugger with

`lldb-rust target/debug/deps/reesecar_engine`

# lldb debug tips

To start debugging from the start try the following

`b main`
`run`
`c`

This will break on the main function, run, and then continue. The first main function will be pure assembly (there are technically two). Continuing will get to the second main function which will be rust code.

To view the assembly code along with the rust code try:

`set set stop-disassembly-display always`


