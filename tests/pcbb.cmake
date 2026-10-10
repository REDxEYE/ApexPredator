add_executable(ApexPcbbTests tests/pcbb_reader.cpp src/apex/pcbb/pcbb.cpp src/utils/lookup3.cpp)
target_include_directories(ApexPcbbTests PRIVATE include)
target_link_libraries(ApexPcbbTests PRIVATE RedsCore SQLiteCpp)
add_test(NAME Apex.PCBB COMMAND ApexPcbbTests)
