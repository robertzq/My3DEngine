cd build
SHC=external/bgfx.cmake/cmake/bgfx/shaderc
mkdir -p bin/assets/shaders_bin
rm -f bin/assets/shaders_bin/vs_simple.bin bin/assets/shaders_bin/fs_simple.bin

# VS —— Metal + varying
"$SHC" -f ../assets/shaders/vs_simple.sc -o bin/assets/shaders_bin/vs_simple.bin \
  --type v --platform osx --profile metal \
  --varying ../assets/shaders/varying.def.sc \
  -i ../external/bgfx.cmake/bgfx/src \
  -i ../external/bgfx.cmake/bgfx/examples/common \
  --preprocess --verbose > bin/assets/shaders_bin/vs_simple.pp.txt

# FS —— Metal + varying（保持一致，尽管这个例子不需要）
"$SHC" -f ../assets/shaders/fs_simple.sc -o bin/assets/shaders_bin/fs_simple.bin \
  --type f --platform osx --profile metal \
  --varying ../assets/shaders/varying.def.sc \
  -i ../external/bgfx.cmake/bgfx/src \
  -i ../external/bgfx.cmake/bgfx/examples/common \
  --preprocess --verbose > bin/assets/shaders_bin/fs_simple.pp.txt

