# simple-cpp-actor  
A simple C++ actor model implement. Used C++14.  
Actor模式的C++实现，最核心的功能已经完整。  
基于C++14标准，因此请使用clang++ v3.5 + libc++ 编译。 

简单的编译说明：  
1. ./bootstrap  
2. CXX="clang++" CXXFLAGS="-stdlib=libc++ -std=c++14" LDFLAGS="-lc++abi" ./configure  
3. make  

请注意，最后编译的test目录，需要googletest的支持，请先安装好googletest，否则这一部分会编译失败。

