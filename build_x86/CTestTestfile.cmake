# CMake generated Testfile for 
# Source directory: /Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）
# Build directory: /Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/build_x86
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test([=[VOXLINEPhase1Tests]=] "/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/build_x86/VOXLINEPhase1Tests_artefacts/Debug/VOXLINEPhase1Tests")
  set_tests_properties([=[VOXLINEPhase1Tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/CMakeLists.txt;151;add_test;/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test([=[VOXLINEPhase1Tests]=] "/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/build_x86/VOXLINEPhase1Tests_artefacts/Release/VOXLINEPhase1Tests")
  set_tests_properties([=[VOXLINEPhase1Tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/CMakeLists.txt;151;add_test;/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test([=[VOXLINEPhase1Tests]=] "/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/build_x86/VOXLINEPhase1Tests_artefacts/MinSizeRel/VOXLINEPhase1Tests")
  set_tests_properties([=[VOXLINEPhase1Tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/CMakeLists.txt;151;add_test;/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test([=[VOXLINEPhase1Tests]=] "/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/build_x86/VOXLINEPhase1Tests_artefacts/RelWithDebInfo/VOXLINEPhase1Tests")
  set_tests_properties([=[VOXLINEPhase1Tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/CMakeLists.txt;151;add_test;/Volumes/Crucial X9/Codex/Voxline（人聲效果鏈）/CMakeLists.txt;0;")
else()
  add_test([=[VOXLINEPhase1Tests]=] NOT_AVAILABLE)
endif()
subdirs("_deps/juce-build")
