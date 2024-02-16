{
  description = "A very basic flake to support vulkan";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs, ... }:
  let
    system = "x86_64-linux";
    pkgs = nixpkgs.legacyPackages.${system};
  in
  {
    devShells.${system}.default =
      pkgs.mkShell
        {
	  nativeBuildInputs = with pkgs; [
	    pkg-config
	  ];
	  buildInputs = [
	    pkgs.vulkan-headers
	    pkgs.vulkan-loader
	    pkgs.vulkan-tools-lunarg
	    pkgs.vulkan-tools
	    pkgs.vulkan-utility-libraries
	    pkgs.vulkan-extension-layer
	    pkgs.vulkan-validation-layers
	    pkgs.glfw
	    pkgs.llvmPackages_17.libcxxabi	#libcxxabi
	    pkgs.llvmPackages_17.libcxx	   	#libcxx
	    pkgs.llvmPackages_17.clangUseLLVM	#clang
	    pkgs.shaderc			#glslc
	  ];

          shellHook = ''
	    echo "Hello Vulkan"
	  '';
	};
  };
}
