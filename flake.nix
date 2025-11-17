{
  description = "A wildfire simulation tool for a university first semester project";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs =
    { nixpkgs, ... }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      formatter.${system} = pkgs.nixfmt-rfc-style;

      devShells.${system}.default = pkgs.mkShell.override { inherit (pkgs.llvmPackages_latest) stdenv; } {
        packages = with pkgs; [
          cmake
          ninja
          llvmPackages_latest.lld
        ];
      };
    };
}
