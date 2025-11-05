{
  description = "Dev shell for C++ project (meson/ninja/g++/openssl/lmdb/nlohmann_json/argparse)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    # при желании можно закрепить другую ветку/тег, например "github:NixOS/nixpkgs/23.11"
  };

  outputs = { self, nixpkgs }: let
    systems = [ "x86_64-linux" ]; # поменяй/дополни если нужно
    mkPkgs = system: import nixpkgs { inherit system; };

    # helper: безопасно выбрать атрибут из nixpkgs (если есть)
    getAttr = pkgs: name:
      if pkgs.lib.hasAttr name pkgs then pkgs.${name} else null;

    chooseOne = pkgs: names:
      let
        existing = builtins.filter (n: pkgs.lib.hasAttr n pkgs) names;
      in if existing == [] then null else pkgs.${builtins.head existing};
  in
  builtins.listToAttrs (map (system:
    let
      pkgs = mkPkgs system;
      lib  = pkgs.lib;

      nlohmannAttr = chooseOne pkgs [ "nlohmann_json" "nlohmann-json" "nlohmannJson" ];
      lmdbAttr = if lib.hasAttr "lmdb" pkgs then pkgs.lmdb else null;
      argparseAttr = chooseOne pkgs [ "argparse" "cpp-argparse" "argparse-p-ranav" "argparse-cpp" ];

      cpp = pkgs.gcc;
      meson = pkgs.meson;
      ninja = pkgs.ninja;
      openssl = pkgs.openssl;
      pkg_nlohmann = if nlohmannAttr != null then nlohmannAttr else null;
      pkg_lmdb = if lmdbAttr != null then lmdbAttr else null;
      pkg_argparse = if argparseAttr != null then argparseAttr else null;

      buildInputsList = lib.filter (x: x != null) [
        meson
        ninja
        cpp
        openssl
        pkg_lmdb
        pkg_nlohmann
        pkg_argparse
      ];
    in
    {
      name = system;
      value = {
        devShell = pkgs.mkShell {
          name = "cpp-dev-shell";

          buildInputs = buildInputsList;

          nativeBuildInputs = [ pkgs.ccache pkgs.cmake pkgs.pkg-config ];

          shellHook = ''
            echo "Dev shell ready for C++ (meson + ninja)."
            echo "Available: meson=$(command -v meson || echo not-found), ninja=$(command -v ninja || echo not-found), g++=$(command -v g++ || echo not-found)"
            echo "To configure build directory: meson setup builddir --buildtype=debugoptimized"
            echo "Then build: ninja -C builddir"
            if [ -n "${toString pkg_nlohmann}" ]; then
              echo "nlohmann_json available in nixpkgs."
            else
              echo "nlohmann_json: not found in this nixpkgs; you may add it or vendor header."
            fi
            if [ -n "${toString pkg_argparse}" ]; then
              echo "argparse (C++) available in nixpkgs as ${toString argparseAttr}."
            else
              echo "argparse (C++) not found in nixpkgs; consider vendoring header or adding custom derivation."
            fi
          '';
        };

        packages = {
          default = pkgs.stdenv.mkDerivation {
            pname = "tulpar";
            version = "0";
            src = null;
            dontBuild = true;
            installPhase = "mkdir -p $out && echo 'use nix develop to enter shell' > $out/README";
          };
        };
      };
    }) systems);
}
