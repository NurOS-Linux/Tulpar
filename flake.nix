{
  description = "tulpar package manager frontend for NurOS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    libapg = {
      url = "git+https://git.nuros.org/core/libapg.git?rev=577aa6fddcc07d3ebab56a545d3166c5b5d605c1";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, libapg }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };
    in
    {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "tulpar";
        version = "0.1.0";

        src = ./.;

        nativeBuildInputs = [
          pkgs.meson
          pkgs.ninja
          pkgs.pkg-config
        ];

        buildInputs = [
          pkgs.curl
          pkgs.yyjson
          pkgs.lmdb
          pkgs.libarchive
          pkgs.gpgme
          pkgs.libsodium
        ];

        postUnpack = ''
          mkdir -p source/subprojects/libapg
          cp -r ${libapg}/. source/subprojects/libapg
          chmod -R u+w source/subprojects/libapg
        '';

        mesonFlags = [
          "--buildtype=release"
        ];

        installPhase = ''
          ninja install
        '';
      };

      devShells.${system}.default = pkgs.mkShell {
        inputsFrom = [ self.packages.${system}.default ];
        packages = [
          pkgs.gdb
          pkgs.valgrind
        ];
      };
    };
}
