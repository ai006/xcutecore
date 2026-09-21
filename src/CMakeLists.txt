add_library(rvcore STATIC
  version.cpp
)
target_include_directories(rvcore PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(rvcore PUBLIC cxx_std_20)
target_link_libraries(rvcore PRIVATE rvsim_options)
