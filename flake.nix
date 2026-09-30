{
  description = "tulpar package manager frontend for NurOS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    libapg = {
      url = "git+https://git.nuros.org/core/libapg.git?rev=4884f63f1ce0830b164d063b275a19f3b995cd12";
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
        version = "0.4.0";

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
