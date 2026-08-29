cmake_minimum_required(VERSION 3.20)
project(new_mavlink VERSION 1.0.0 LANGUAGES C CXX)

option(NEWMAVLINK_SANITIZERS "Enable ASan and UBSan" OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NEWMAVLINK_SANITIZERS)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

set(ASCON_AEAD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ascon-c/crypto_aead/ascon128av13/opt64")
set(ASCON_PRF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ascon-c/crypto_auth/asconprfv13/opt64")
add_library(newmavlink_ascon STATIC
  "${ASCON_AEAD_DIR}/aead.c"
  "${ASCON_AEAD_DIR}/permutations.c"
  "${ASCON_PRF_DIR}/prf.c"
)
target_include_directories(newmavlink_ascon PRIVATE
  "${ASCON_AEAD_DIR}"
  "${ASCON_PRF_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ascon-c/tests"
)
target_compile_options(newmavlink_ascon PRIVATE -Wall -Wextra -Wpedantic)

add_library(newmavlink_core STATIC
  src/crc16.cpp
  src/crypto.cpp
  src/record.cpp
  src/wire.cpp
  src/messages.cpp
  src/session.cpp
  src/transport.cpp
  src/qos.cpp
  src/proxy.cpp
  src/publisher.cpp
)
target_include_directories(newmavlink_core PUBLIC include PRIVATE
  "${ASCON_AEAD_DIR}"
  "${ASCON_PRF_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/ascon-c/tests"
)
target_link_libraries(newmavlink_core PRIVATE newmavlink_ascon)
target_compile_options(newmavlink_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

if(NEWMAVLINK_SANITIZERS)
  target_compile_options(newmavlink_ascon PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(newmavlink_ascon PRIVATE -fsanitize=address,undefined)
  target_compile_options(newmavlink_core PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(newmavlink_core PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()
function(newmavlink_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE newmavlink_core)
  target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -UNDEBUG)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

newmavlink_test(newmavlink_wire_test tests/wire_test.cpp)
newmavlink_test(newmavlink_messages_test tests/messages_test.cpp)
newmavlink_test(newmavlink_session_test tests/session_test.cpp)
newmavlink_test(newmavlink_qos_test tests/qos_test.cpp)
newmavlink_test(newmavlink_proxy_test tests/proxy_test.cpp)
newmavlink_test(newmavlink_security_test tests/security_test.cpp)
newmavlink_test(newmavlink_fuzz_test tests/fuzz_test.cpp)
newmavlink_test(newmavlink_messaging_advanced_test tests/messaging_advanced_test.cpp)
newmavlink_test(newmavlink_publisher_test tests/publisher_test.cpp)

add_executable(newmavlink_vehicle apps/vehicle_endpoint.cpp)
target_link_libraries(newmavlink_vehicle PRIVATE newmavlink_core)
add_executable(newmavlink_gcs apps/gcs_endpoint.cpp)
target_link_libraries(newmavlink_gcs PRIVATE newmavlink_core)
add_executable(newmavlink_proxy apps/proxy_service.cpp)
target_link_libraries(newmavlink_proxy PRIVATE newmavlink_core)
add_executable(newmavlink_publisher_demo apps/publisher_demo.cpp)
target_link_libraries(newmavlink_publisher_demo PRIVATE newmavlink_core)

add_executable(newmavlink_px4_probe apps/px4_adapter_probe.cpp)
target_link_libraries(newmavlink_px4_probe PRIVATE newmavlink_mavlink2_adapter)
target_include_directories(newmavlink_px4_probe PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mavlink-c-library/common" "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mavlink-c-library")

add_library(newmavlink_mavlink2_adapter STATIC src/mavlink2_adapter.cpp)
target_include_directories(newmavlink_mavlink2_adapter PUBLIC include PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mavlink-c-library/common"
  "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mavlink-c-library"
)
target_link_libraries(newmavlink_mavlink2_adapter PUBLIC newmavlink_core)
target_compile_options(newmavlink_mavlink2_adapter PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

foreach(target newmavlink_vehicle newmavlink_gcs newmavlink_proxy newmavlink_publisher_demo)
  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endforeach()
add_test(NAME newmavlink_process_e2e COMMAND "${CMAKE_COMMAND}" -E env "NEWMAVLINK_BUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}" bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/process_e2e.sh")
add_executable(newmavlink_adapter_test tests/adapter_test.cpp)
target_link_libraries(newmavlink_adapter_test PRIVATE newmavlink_mavlink2_adapter)
target_include_directories(newmavlink_adapter_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mavlink-c-library/common" "${CMAKE_CURRENT_SOURCE_DIR}/third_party/mavlink-c-library")
target_compile_options(newmavlink_adapter_test PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -UNDEBUG)
add_test(NAME newmavlink_adapter_test COMMAND newmavlink_adapter_test)

install(DIRECTORY include/ DESTINATION include)
