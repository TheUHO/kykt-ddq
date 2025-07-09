
//TODO: 这个文件用于放置各种初始化时要读入的算子

// oplib_load_builtin(aaaa);
// oplib_load_so("...", "...");

// #ifdef ...
// #include "...."
// #endif

#ifdef enable_processor_tianhe

oplib_init_tianhe();

oplib_load_tianheDat("/thfs1/home/lzz/hanbin/DSQ/ddq/compiled/op_tianhe_kernel.dat");
#endif


