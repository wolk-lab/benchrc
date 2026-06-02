{
  inputs = {
    nixpkgs.url = "github:cachix/devenv-nixpkgs/rolling";
    systems.url = "github:nix-systems/default";
    devenv.url = "github:cachix/devenv";
    devenv.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = inputs@{ self, nixpkgs, systems, devenv, ... }:
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
                  gbenchmark
                  llvmPackages.clang
                  llvmPackages.openmp
                  cmake
                  gnumake
                  ninja
                  pkg-config
                ];

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
