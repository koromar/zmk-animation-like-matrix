# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

# Add the matrix animation source file
zephyr_library_sources(matrix.c)

# Conditionally add the widget source file if enabled via Kconfig
if(CONFIG_CUSTOM_WIDGET_MATRIX)
    zephyr_library_sources(widgets/matrix_widget.c)
    # Add any specific include directories needed by the widget
    # zephyr_library_include_directories(include)
endif() 