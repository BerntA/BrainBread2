# Compiling Shaders

## Usage
```
buildshaders.exe [OPTIONS] -ver n -shaderdir src_dir shader.fxc
buildshaders-force.bat
buildshaders-force-dynamic.bat
```
## Options
```
-ver ARG                       Sets shader version, required
-shaderpath ARG                Base path for shaders, required
-crc                           Calculate crc for shader
-dynamic                       Generate only header
-force                         Skip crc check during compilation
-threads ARG                   Number of threads used, defaults to core count

-h, -help                      Shows help
-verbose                       Verbose file cache and final shader info
-verbose2                      Verbose compile commands
-verbose_preprocessor          Enables preprocessor debug printing

-disable-optimization, /Od     Disables shader optimization
-disable-preshader, /Op        Disables preshader generation
-no-flow-control, /Gfa         Directs the compiler to not use flow-control constructs where possible
-prefer-flow-control, /Gfp     Directs the compiler to use flow-control constructs where possible
-partial-precision, /Gpp       Compiles shader with partial precission
-no-validation, /Vd            Skips shader validation
```
