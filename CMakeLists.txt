#
# Application specific CMakeLists.txt file
#   Included from vendors/cypress/psoc6/cmake/cy_kit_utils.cmake
#
# Add app specific sources, defines, includes here
#
#--------------------------------------------------------------------
# This app dir
get_filename_component(CY_APP_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

#set(COMPONENTS "$(COMPONENTS);EMWIN_OSNTS" CACHE STRING "" FORCE)
set(EMWIN_VFP_SELECT "SOFTFP")
set(EMWIN_LIB_SELECT "EMWIN_OSNTS")
set(EMWIN_DIR "${CMAKE_CURRENT_LIST_DIR}/libs/emwin")

#--------------------------------------------------------------------
# Additional defines
target_compile_definitions(${AFR_TARGET_APP_NAME} PUBLIC
    "-DCY_RETARGET_IO_CONVERT_LF_TO_CRLF"
    )

# Application sources
# ( main.c already included in vendors/cypress/psoc6/cmake/cy_defines.cmake)
# All the source files in this example directory are recursively added through
# cy_kit_generate() function in vendors/cypress/psoc6/cmake/cy_kit_utils.cmake.
# target_sources(${AFR_TARGET_APP_NAME} PUBLIC "" )
target_sources(${AFR_TARGET_APP_NAME} PUBLIC 
    "${AFR_DEMOS_DIR}/demo_runner/aws_demo.c"
    "${AFR_DEMOS_DIR}/demo_runner/iot_demo_freertos.c"
    "${AFR_DEMOS_DIR}/dev_mode_key_provisioning/src/aws_dev_mode_key_provisioning.c"
    "${AFR_DEMOS_DIR}/network_manager/aws_iot_network_manager.c"
)

# Additional include dirs
# target_include_directories(${AFR_TARGET_APP_NAME} PUBLIC "" )
target_include_directories(${AFR_TARGET_APP_NAME} PUBLIC 
    "${CMAKE_CURRENT_LIST_DIR}/source"
    "${CMAKE_CURRENT_LIST_DIR}/source/cJSON"
    "${CMAKE_CURRENT_LIST_DIR}/source/cy8ckit_032_oled"
    "${CMAKE_CURRENT_LIST_DIR}/source/cy8ckit_032_oled/driver"
    "${CMAKE_CURRENT_LIST_DIR}/source/cy8ckit_032_oled/emwin_config"
    "${CMAKE_CURRENT_LIST_DIR}/libs/emwin/GUI/Include"
    "${EMWIN_DIR}/GUI/COMPONENT_${EMWIN_VFP_SELECT}/COMPONENT_${EMWIN_LIB_SELECT}"
    "${AFR_DEMOS_DIR}/dev_mode_key_provisioning/include" 
    "${AFR_DEMOS_DIR}/network_manager"
    "${AFR_MODULES_DIR}/3rdparty/pkcs11"
    "${AFR_MODULES_FREERTOS_PLUS_DIR}/standard/pkcs11/include"
    "${AFR_MODULES_ABSTRACTIONS_DIR}/platform/include/platform"
    "${AFR_MODULES_C_SDK_DIR}/standard/mqtt/include"
)              

# AWS Configuration directory. Set if CE is not owning config_files (i.e when  
# config_files directory is not present inside the CE directory). 
# When the path is empty, config_files directory inside the CE directory will be
# used.
# set(CY_APP_CONFIG_DIR "${cy_board_dir}/aws_demos/config_files")

# AWS IOT Configuration directory. Set if CE is not owning include directory. 
# (i.e when include directory is not present inside the CE directory.). When the
# path is empty, include directory inside the CE directory will be used. 
set(CY_APP_IOT_CONFIG_DIR "${AFR_DEMOS_DIR}/include")

# Additional libs
# target_link_libraries(${AFR_TARGET_APP_NAME} PUBLIC "<lib_name>" )

if("${AFR_TOOLCHAIN}" STREQUAL "arm-gcc")
    set(LIB_EXTN "a")
    set(TOOLCHAIN GCC_ARM)
elseif("${AFR_TOOLCHAIN}" STREQUAL "arm-armclang")
    set(LIB_EXTN "ar")
    set(TOOLCHAIN ARM)
elseif("${AFR_TOOLCHAIN}" STREQUAL "arm-iar")
    set(LIB_EXTN "a")
    set(TOOLCHAIN IAR)
endif()

file(GLOB CY_APP_LIBS
    "${EMWIN_DIR}/GUI/COMPONENT_${EMWIN_VFP_SELECT}/COMPONENT_${EMWIN_LIB_SELECT}/TOOLCHAIN_${TOOLCHAIN}/*.${LIB_EXTN}"
)
    
target_link_libraries(${AFR_TARGET_APP_NAME} PUBLIC
    "AFR::mqtt"
    "${CY_APP_LIBS}"
)  

# Extra linker options
# target_link_options(${AFR_TARGET_APP_NAME} PUBLIC "" )

#--------------------------------------------------------------------
# Disable AFR aws_demos Modules
# These AFR aws_demos modules are not needed for the weather station to build
set(CY_APP_DISABLE_AFR_MODULES
    "defender"
    "greengrass"
    "shadow"
    "serializer"
    )

