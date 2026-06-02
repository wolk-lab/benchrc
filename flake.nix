{
  inputs = {
    nixpkgs.url = "github:cachix/devenv-nixpkgs/rolling";
    systems.url = "github:nix-systems/default";
    devenv.url = "github:cachix/devenv";
    devenv.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { nixpkgs, systems, devenv, ... } @ inputs:
    let
      forEachSystem = nixpkgs.lib.genAttrs (import systems);
    in
    {
      devShells = forEachSystem (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in
        {
          default = devenv.lib.mkShell {
            inherit inputs pkgs;
            modules = [
              ({ pkgs, ... }: {
                packages = with pkgs; [
                  cmake
                  gbenchmark
                  gnumake
                  llvmPackages.clang
                  llvmPackages.openmp
                  ninja
                  pkg-config
                  taskflow
                ];

                env.CPLUS_INCLUDE_PATH = "${pkgs.taskflow}/include";
                env.CMAKE_INCLUDE_PATH = "${pkgs.taskflow}/include";

                languages.rust.enable = true;

                enterShell = ''
                  echo "Rust benchmarks: cargo bench --bench benchmarks"
                  echo "C++ benchmarks: cmake -S cpp -B cpp/build && cmake --build cpp/build --target seminar_google_benchmarks"
                '';
              })
            ];
          };
        });
    };
}
