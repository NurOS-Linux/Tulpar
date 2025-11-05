{
  description = "Tulpar C++ project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
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
          pkgs.gcc
          pkgs.openssl
          pkgs.lmdb
          pkgs.lmdbxx
          pkgs.libarchive
          pkgs.argparse
          pkgs.nlohmann_json
        ];

        mesonFlags = [
          "--buildtype=release"
        ];

        installPhase = ''
          mkdir -p $out/bin $out/lib
          cp -r apg*/apg* $out/bin
          cp -r libapg/libapg.so $out/lib
        '';
      };
    };
}
