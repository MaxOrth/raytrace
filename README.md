# raytrace

Build instructions:

1. Run premake5 from the project root. For visual studio, use vs2015 or vs2017 as the argument. For makefile, use gmake I think?
2. Build files are in ./build. If you are in visual studio, you might need to set the windows sdk version.  You will know because the compiler will complain about missing standard header files.
3. If you are windows I have a helper program that lists opencl devices.  Its in one of the \_old directories, called list_devices.exe. Run that from a terminal.
4. Run the executable from the project root.  Exe is somewhere, I forgot.  You might need to do this from a terminal.  If you have multiple opencl-capable devices, then you will need to specify which device to run on.  The first argument is platform, the second is device.  Output is formatted like this:

```
amd platform
  amd ryzen 1700 list_of_extensions
nvidia platform
  nvidida gtx 1080 list_of_extensions
```
   
You would use: 1 0 as the arguments b/c the gpu is platform 2 (index 1) and the device is index 0.  You can also try other devices to see what happens :)
