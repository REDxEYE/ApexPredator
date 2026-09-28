add_executable(ApexRtpcTests tests/rtpc_reader.cpp src/apex/rtpc.cpp src/utils/lookup3.cpp)
target_include_directories(ApexRtpcTests PRIVATE include)
target_link_libraries(ApexRtpcTests PRIVATE RedsCore)
add_test(NAME Apex.RTPC COMMAND ApexRtpcTests)
