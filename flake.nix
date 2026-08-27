{
  description = "tulpar package manager frontend for NurOS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    libapg = {
      url = "git+https://git.nuros.org/core/libapg.git?rev=191635fce90f4c9bdc70f974db9cf8f6c2e3eb0c";
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
        version = "0.2.0";

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
