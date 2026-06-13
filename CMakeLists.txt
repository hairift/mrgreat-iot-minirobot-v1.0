# Baris boilerplate berikut harus ada di
# CMakeLists proyek dengan urutan tepat agar CMake bekerja benar
cmake_minimum_required(VERSION 3.16)

# Tambahkan baris ini untuk menonaktifkan peringatan tertentu
add_compile_options(-Wno-missing-field-initializers)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# Ringkas build. Sertakan himpunan komponen minimum, `main`, dan semua dependensinya.
idf_build_set_property(MINIMAL_BUILD ON)

set(PROJECT_VER "2.2.6")
project(xiaozhi)
