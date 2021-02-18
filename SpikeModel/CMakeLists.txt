project(SPIKE_MODEL)

include (../ExamplesMacros.cmake)


add_executable(spike_model
  src/main.cpp
  src/SpikeModel.cpp
  src/CPUFactory.cpp
  src/CPUTopology.cpp
  src/CPU.cpp
  src/Core.cpp
  src/L2Cache.cpp
  src/NoC.cpp
  $ENV{SPIKE_PATH}/spike_main/spike_wrapper.cc
  )
sparta_application(spike_model)

target_link_libraries(spike_model $ENV{SPIKE_PATH}/build/libspike_main.a  $ENV{SPIKE_PATH}/build/libriscv.a  $ENV{SPIKE_PATH}/build/libsoftfloat.a  $ENV{SPIKE_PATH}/build/libfesvr.a -ldl)
include_directories(SYSTEM $ENV{SPIKE_PATH}/riscv/ $ENV{SPIKE_PATH}/build $ENV{SPIKE_PATH}/softfloat $ENV{SPIKE_PATH}/ $ENV{SPIKE_PATH}/spike_main/ $ENV{SPIKE_MODEL_SRC})


# Add the custom regress/regress_valgrind targets.
add_custom_target (spike_model_regress)
add_custom_target (spike_model_regress_valgrind)

# NOTE:
# running ctest with --test-action test creates Testing/<datetime>/Test.xml
# that can be loaded into the CI test result tracker
add_custom_command(TARGET spike_model_regress          POST_BUILD COMMAND ctest -LE ${VALGRIND_TEST_LABEL} -j10 --test-action test)
add_custom_command(TARGET spike_model_regress_valgrind POST_BUILD COMMAND ctest -L  ${VALGRIND_TEST_LABEL} -j10 --test-action test)

