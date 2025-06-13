{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    pkgs.clang
    pkgs.meson
    pkgs.argparse
    pkgs.nlohmann_json
    pkgs.ninja
  ];

  shellHook = "
    export CXX=clang++
  ";
}
