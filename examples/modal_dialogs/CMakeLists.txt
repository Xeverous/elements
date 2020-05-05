###############################################################################
#  Copyright (c) 2020 Michal Urbanski
#
#  Distributed under the MIT License (https://opensource.org/licenses/MIT)
###############################################################################
cmake_minimum_required(VERSION 3.9.6...3.15.0)
project(ModalDialogs LANGUAGES C CXX)

if (NOT ELEMENTS_ROOT)
   message(FATAL_ERROR "ELEMENTS_ROOT is not set")
endif()

# If we are building outside the project, you need to set ELEMENTS_ROOT:
if (NOT ELEMENTS_BUILD_EXAMPLES)
   include(${ELEMENTS_ROOT}/cmake/CMakeInclude.txt)
   set(ELEMENTS_BUILD_EXAMPLES OFF)
   add_subdirectory(${ELEMENTS_ROOT} elements)
endif()

set(ELEMENTS_APP_PROJECT "ModalDialogs")
set(ELEMENTS_APP_TITLE "Modal Dialogs")
set(ELEMENTS_APP_COPYRIGHT "Copyright (c) 2020 Michal Urbanski")
set(ELEMENTS_APP_ID "com.cycfi.modal-dialogs")
set(ELEMENTS_APP_VERSION "1.0")

include("${ELEMENTS_ROOT}/CMakeMain.txt")
