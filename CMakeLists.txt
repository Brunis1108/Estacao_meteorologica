# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Estacao_metereologica C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Estacao_metereologica Estacao_metereologica.c lib/aht20.c lib/bmp280.c lib/ssd1306.c lib/src/web_Site.c)


pico_set_program_name(Estacao_metereologica "Estacao_metereologica")
pico_set_program_version(Estacao_metereologica "0.1")

pico_generate_pio_header(Estacao_metereologica ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Estacao_metereologica 1)
pico_enable_stdio_usb(Estacao_metereologica 1)

# Add the standard library to the build
target_link_libraries(Estacao_metereologica
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(Estacao_metereologica PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)
target_include_directories(Estacao_metereologica PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib/headers
        ${CMAKE_CURRENT_LIST_DIR}/lib
)

# Add any user requested libraries
target_link_libraries(Estacao_metereologica 
        hardware_i2c
        hardware_gpio
        hardware_adc
        pico_time
        pico_cyw43_arch_lwip_threadsafe_background
        hardware_pwm
        )

pico_add_extra_outputs(Estacao_metereologica)

