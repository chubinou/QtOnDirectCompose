cmake_minimum_required(VERSION 2.8)

project(DComposition)

#find_package(Qt5 COMPONENTS Core Quick QuickWidgets Gui Widgets REQUIRED)
find_package(PkgConfig)

pkg_check_modules(QT5 "Qt5Core" "Qt5Gui" "Qt5QuickControls2" "Qt5Widgets" "Qt5Quick" "Qt5QuickWidgets" REQUIRED)

add_library("Pkg::Qt5" INTERFACE IMPORTED GLOBAL)
set_target_properties("Pkg::Qt5" PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${QT5_STATIC_INCLUDE_DIRS}")

set(link_libs)
list(APPEND link_libs ${QT5_STATIC_LDFLAGS})
list(APPEND link_libs ${QT5_STATIC_LDFLAGS_OTHER})
set_target_properties("Pkg::Qt5" PROPERTIES INTERFACE_LINK_LIBRARIES "${link_libs}")

set(compile_opts)
list(APPEND compile_opts ${QT5_STATIC_LDFLAGS})
list(APPEND compile_opts ${QT5_STATIC_LDFLAGS_OTHER})
set_target_properties("Pkg::Qt5" PROPERTIES INTERFACE_COMPILE_OPTIONS "${compile_opts}")

find_program(RCC rcc)

function(_QT5_PARSE_QRC_FILE infile _rc_depends)
    get_filename_component(rc_path ${infile} PATH)

    if(EXISTS "${infile}")
        #  parse file for dependencies
        #  all files are absolute paths or relative to the location of the qrc file
        file(READ "${infile}" RC_FILE_CONTENTS)
        string(REGEX MATCHALL "<file[^<]+" RC_FILES "${RC_FILE_CONTENTS}")
        foreach(RC_FILE ${RC_FILES})
            string(REGEX REPLACE "^<file[^>]*>" "" RC_FILE "${RC_FILE}")
            if(NOT IS_ABSOLUTE "${RC_FILE}")
                set(RC_FILE "${rc_path}/${RC_FILE}")
            endif()
            set(RC_DEPENDS ${RC_DEPENDS} "${RC_FILE}")
        endforeach()

    else()
        # The .qrc file does not exist (yet). Let's add a dependency and hope
        # that it will be generated later
        set(out_depends)
    endif()

    set(${_rc_depends} ${RC_DEPENDS} PARENT_SCOPE)
endfunction()


function(QT5_ADD_RESOURCES outfiles )

    set(options)
    set(oneValueArgs)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(_RCC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(rcc_files ${_RCC_UNPARSED_ARGUMENTS})
    set(rcc_options ${_RCC_OPTIONS})

    if("${rcc_options}" MATCHES "-binary")
        message(WARNING "Use qt5_add_binary_resources for binary option")
    endif()

    foreach(it ${rcc_files})
        get_filename_component(outfilename ${it} NAME_WE)
        get_filename_component(infile ${it} ABSOLUTE)
        set(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.cpp)

        _QT5_PARSE_QRC_FILE(${infile} _rc_depends)
        set_source_files_properties(${infile} PROPERTIES SKIP_AUTORCC ON)

        add_custom_command(OUTPUT ${outfile}
            COMMAND ${RCC}
            ARGS ${rcc_options} --name ${outfilename} --output ${outfile} ${infile}
            MAIN_DEPENDENCY ${infile}
            DEPENDS ${_rc_depends} VERBATIM)

        list(APPEND ${outfiles} ${outfile})
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()

QT5_ADD_RESOURCES(QRC_SOURCES resource.qrc)
add_executable(${PROJECT_NAME} "main.cpp" "compositor.cpp" "compositor.hpp" ${QRC_SOURCES})

target_link_libraries(${PROJECT_NAME} Pkg::Qt5 dcomp d3d11)

add_executable("simpleegl" "simpleegl.cpp")
target_link_libraries(${PROJECT_NAME} Pkg::Qt5 dcomp d3d11)

