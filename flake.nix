
{
  description = "Development environment for C++ System Monitor";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

  outputs =
    { nixpkgs, ... }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
    in
    {
      devShells = nixpkgs.lib.genAttrs systems (
        system:
        let
          pkgs = import nixpkgs { inherit system; };
          lib = pkgs.lib;

          # Legacy Udacity code accesses WINDOW->_maxx/_maxy.
          # ncurses >= 6.5 uses opaque WINDOW by default.
          ncursesLegacy = pkgs.ncurses.overrideAttrs (old: {
            configureFlags = old.configureFlags ++ [
              "--disable-opaque-curses"
            ];
          });
        in
        {
          default = pkgs.mkShell {
            name = "udacity-system-monitor";

            packages = [
              pkgs.cmake
              pkgs.gnumake
              pkgs.pkg-config
              pkgs.clang-tools
            ]
            ++ lib.optionals pkgs.stdenv.isLinux [
              pkgs.gdb
            ];

            buildInputs = [
              ncursesLegacy
            ];

            # Support old CMake projects.
            CMAKE_POLICY_VERSION_MINIMUM = "3.10";

            # Direct CMake toward the Nix-provided ncurses.
            CMAKE_PREFIX_PATH = "${ncursesLegacy.dev}";

            CURSES_INCLUDE_PATH = "${ncursesLegacy.dev}/include";
            CURSES_LIBRARY = "${ncursesLegacy.out}/lib/libncurses.so";

            # Prefer the Nix ncurses headers and libraries
            # when compiling directly outside CMake.
            NIX_CFLAGS_COMPILE = "-I${ncursesLegacy.dev}/include -Wno-error=format-security";
            NIX_LDFLAGS = "-L${ncursesLegacy.out}/lib";

            shellHook = ''
              echo "C++ System Monitor development environment"
              echo "ncurses: ${ncursesLegacy.version} (non-opaque)"
            ''
            + lib.optionalString pkgs.stdenv.isDarwin ''
              echo "Note: monitor requires Linux /proc."
              echo "Run it in a Linux VM or container."
            '';
          };
        }
      );
    };
}
