// Stand-in for the name of Vivado's simulator kernel, before 2025.1, when
// librdi_simulator_kernel.so had no soname. A design NEEDs the kernel by that
// name, and the loader matches a NEEDED entry only to a loaded library's
// soname (or the name it was loaded by, for the kernel a full path). This
// empty library has the name, and NEEDs the kernel by its path: see
// load_closure(), which writes the path in.
