# CMake generated Testfile for 
# Source directory: /home/ubuntu/Downloads/new_mavlink_run_20260827_094328
# Build directory: /home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[newmavlink_wire_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_wire_test")
set_tests_properties([=[newmavlink_wire_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;62;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;65;newmavlink_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_messages_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_messages_test")
set_tests_properties([=[newmavlink_messages_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;62;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;66;newmavlink_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_session_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_session_test")
set_tests_properties([=[newmavlink_session_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;62;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;67;newmavlink_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_qos_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_qos_test")
set_tests_properties([=[newmavlink_qos_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;62;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;68;newmavlink_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_proxy_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_proxy_test")
set_tests_properties([=[newmavlink_proxy_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;62;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;69;newmavlink_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_security_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_security_test")
set_tests_properties([=[newmavlink_security_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;62;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;70;newmavlink_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_fuzz_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_fuzz_test")
set_tests_properties([=[newmavlink_fuzz_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;62;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;71;newmavlink_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_messaging_advanced_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_messaging_advanced_test")
set_tests_properties([=[newmavlink_messaging_advanced_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;62;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;72;newmavlink_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_publisher_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_publisher_test")
set_tests_properties([=[newmavlink_publisher_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;62;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;73;newmavlink_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_process_e2e]=] "/usr/bin/cmake" "-E" "env" "NEWMAVLINK_BUILD_DIR=/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build" "bash" "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/tests/process_e2e.sh")
set_tests_properties([=[newmavlink_process_e2e]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;99;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
add_test([=[newmavlink_adapter_test]=] "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/build/newmavlink_adapter_test")
set_tests_properties([=[newmavlink_adapter_test]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;104;add_test;/home/ubuntu/Downloads/new_mavlink_run_20260827_094328/CMakeLists.txt;0;")
