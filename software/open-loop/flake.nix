{
  description = "ESP-IDF development shell";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    nixpkgs-esp-dev = {
      url = "github:mirrexagon/nixpkgs-esp-dev";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, nixpkgs-esp-dev }:
    let
      system = builtins.currentSystem; 

      pkgs = import nixpkgs {
        inherit system;

        overlays = [
          (import "${nixpkgs-esp-dev}/overlay.nix")
        ];

        config.permittedInsecurePackages = [
          "python3.13-ecdsa-0.19.1"
        ];
      };
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        name = "esp-project";

        buildInputs = with pkgs; [
          esp-idf-full
        ];
      };
    };
}

