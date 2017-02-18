################################################################################
##
##  Provide Pandoc compilation support for the CMake build system
##
##  Version: 0.0.1-dirty
##  Author: Jeet Sukumatan (jeetsukumaran@gmail.com)
##          Jerry Jacobs (jerry.jacobs@xor-gate.org)
##
##  Copyright 2015 Jeet Sukumaran.
##  Copyright 2016 Jerry Jacobs
##
##  This software is released under the BSD 3-Clause License.
##
##  Redistribution and use in source and binary forms, with or without
##  modification, are permitted provided that the following conditions are
##  met:
##
##  1. Redistributions of source code must retain the above copyright notice,
##  this list of conditions and the following disclaimer.
##
##  2. Redistributions in binary form must reproduce the above copyright
##  notice, this list of conditions and the following disclaimer in the
##  documentation and/or other materials provided with the distribution.
##
##  3. Neither the name of the copyright holder nor the names of its
##  contributors may be used to endorse or promote products derived from this
##  software without specific prior written permission.
##
##  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
##  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
##  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
##  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
##  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
##  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
##  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
##  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
##  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
##  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
##  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
################################################################################

include(CMakeParseArguments)

if(NOT EXISTS ${PANDOC_EXECUTABLE})
    find_program(PANDOC_EXECUTABLE pandoc)
    mark_as_advanced(PANDOC_EXECUTABLE)
endif()

###############################################################################
# Based on code from UseLATEX
# Author: Kenneth Moreland <kmorel@sandia.gov>
# Copyright 2004 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the
# U.S. Government. Redistribution and use in source and binary forms, with
# or without modification, are permitted provided that this Notice and any
# statement of authorship are reproduced on all copies.

# Adds command to copy file from the source directory to the destination
# directory: used to move source files from source directory into build
# directory before main build
function(pandocology_add_input_file source_path dest_dir dest_filelist_var)
    set(dest_filelist)
    file(GLOB globbed_source_paths "${source_path}")
    foreach(globbed_source_path ${globbed_source_paths})
        # MESSAGE(FATAL_ERROR "${globbed_source_path}")
        get_filename_component(filename ${globbed_source_path} NAME)
        get_filename_component(absolute_dest_path ${dest_dir}/${filename} ABSOLUTE)
        file(RELATIVE_PATH relative_dest_path ${CMAKE_CURRENT_BINARY_DIR} ${absolute_dest_path})
        list(APPEND dest_filelist ${absolute_dest_path})
        ADD_CUSTOM_COMMAND(
            OUTPUT ${relative_dest_path}
            COMMAND ${CMAKE_COMMAND} -E copy ${globbed_source_path} ${dest_dir}/${filename}
            DEPENDS ${globbed_source_path}
            )
        set(${dest_filelist_var} ${${dest_filelist_var}} ${dest_filelist} PARENT_SCOPE)
    endforeach()
endfunction()

# A version of GET_FILENAME_COMPONENT that treats extensions after the last
# period rather than the first.
function(pandocology_get_file_stemname varname filename)
    SET(result)
    GET_FILENAME_COMPONENT(name ${filename} NAME)
    STRING(REGEX REPLACE "\\.[^.]*\$" "" result "${name}")
    SET(${varname} "${result}" PARENT_SCOPE)
endfunction()

function(pandocology_get_file_extension varname filename)
    SET(result)
    GET_FILENAME_COMPONENT(name ${filename} NAME)
    STRING(REGEX MATCH "\\.[^.]*\$" result "${name}")
    SET(${varname} "${result}" PARENT_SCOPE)
endfunction()
###############################################################################

function(pandocology_add_input_dir source_dir dest_parent_dir dir_dest_filelist_var)
    set(dir_dest_filelist)
    get_filename_component(dir_name ${source_dir} NAME)
    get_filename_component(absolute_dest_dir ${dest_parent_dir}/${dir_name} ABSOLUTE)
    file(RELATIVE_PATH relative_dest_dir ${CMAKE_CURRENT_BINARY_DIR} ${absolute_dest_dir})
    add_custom_command(
        OUTPUT ${relative_dest_dir}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${absolute_dest_dir}
        DEPENDS ${source_dir}
        )
    file(GLOB source_files "${source_dir}/*")
    foreach(source_file ${source_files})
        # get_filename_component(absolute_source_path ${CMAKE_CURRENT_SOURCE_DIR}/${source_file} ABSOLUTE)
        pandocology_add_input_file(${source_file} ${absolute_dest_dir} dir_dest_filelist)
    endforeach()
    set(${dir_dest_filelist_var} ${${dir_dest_filelist_var}} ${dir_dest_filelist} PARENT_SCOPE)
endfunction()

function(add_to_make_clean filepath)
    get_directory_property(make_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${make_clean_files};${filepath}")
endfunction()

function(disable_insource_build)
    IF ( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
        MESSAGE(FATAL_ERROR "The build directory must be different from the main project source "
"directory. Please create a directory such as '${CMAKE_SOURCE_DIR}/build', "
"and run CMake from there, passing the path to this source directory as "
"the path argument. E.g.:
    $ cd ${CMAKE_SOURCE_DIR}
    $ mkdir build
    $ cd build
    $ cmake .. && make && sudo make install
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them:
    $ rm -r CMakeFiles/ CmakeCache.txt
")
    ENDIF()
endfunction()

# This builds a document
#
# Usage:
#
#
#     INCLUDE(pandocology)
#
#     add_document(
#         figures.tex
#         SOURCES              figures.md
#         RESOURCE_DIRS        figs
#         PANDOC_DIRECTIVES    -t latex
#         NO_EXPORT_PRODUCT
#         )
#
#     add_document(
#         opus.pdf
#         SOURCES              opus.md
#         RESOURCE_FILES       opus.bib systbiol.template.latex systematic-biology.csl
#         RESOURCE_DIRS        figs
#         PANDOC_DIRECTIVES    -t             latex
#                             --smart
#                             --template     systbiol.template.latex
#                             --filter       pandoc-citeproc
#                             --csl          systematic-biology.csl
#                             --bibliography opus.bib
#                             --include-after-body=figures.tex
#         DEPENDS             figures.tex
#         EXPORT_ARCHIVE
#         )
#
function(add_document target_name)
    if (NOT PANDOC_EXECUTABLE)
       message(WARNING "Pandoc not found. Install Pandoc (http://johnmacfarlane.net/pandoc/) or set cache variable PANDOC_EXECUTABLE.")
       return()
    endif()

    set(options          EXPORT_ARCHIVE NO_EXPORT_PRODUCT EXPORT_PDF DIRECT_TEX_TO_PDF VERBOSE)
    set(oneValueArgs     PRODUCT_DIRECTORY)
    set(multiValueArgs   SOURCES RESOURCE_FILES RESOURCE_DIRS PANDOC_DIRECTIVES DEPENDS)
    cmake_parse_arguments(ADD_DOCUMENT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    # this is because `make clean` will dangerously clean up source files
    disable_insource_build()

    # get the stem of the target name
    pandocology_get_file_stemname(target_stemname ${target_name})
    pandocology_get_file_extension(target_extension ${target_name})
    if (${ADD_DOCUMENT_EXPORT_PDF})
        if (NOT "${target_extension}" STREQUAL ".tex" AND NOT "${target_extension}" STREQUAL ".latex")
        # if (NOT "${target_extension}" STREQUAL ".tex")
            MESSAGE(FATAL_ERROR "Target '${target_name}': Cannot use 'EXPORT_PDF' for target of type '${target_extension}': target type must be '.tex' or '.latex'")
        endif()
    endif()
    if (${ADD_DOCUMENT_DIRECT_TEX_TO_PDF})
        list(LENGTH ${ADD_DOCUMENT_SOURCES} SOURCE_LEN)
        if (SOURCE_LEN GREATER 1)
            MESSAGE(FATAL_ERROR "Target '${target_name}': Only one source can be specified when using the 'DIRECT_TEX_TO_PDF' option")
        endif()
        # set(ADD_DOCUMENT_SOURCES, list(GET ${ADD_DOCUMENT_SOURCES} 1))
        pandocology_get_file_stemname(source_stemname ${ADD_DOCUMENT_SOURCES})
        pandocology_get_file_extension(source_extension ${ADD_DOCUMENT_SOURCES})
        if (NOT "${source_extension}" STREQUAL ".tex" AND NOT "${source_extension}" STREQUAL ".latex")
            MESSAGE(FATAL_ERROR "Target '${target_name}': Cannot use 'DIRECT_TEX_TO_PDF' for source of type '${source_extension}': source type must be '.tex' or '.latex'")
        endif()
        SET(check_target ${source_stemname}.pdf)
        IF (NOT ${check_target} STREQUAL ${target_name})
            MESSAGE(FATAL_ERROR "Target '${target_name}': Must use target name of '${check_target}' if using 'DIRECT_TEX_TO_PDF'")
        endif()
    endif()

    ## set up output directory
    if ("${ADD_DOCUMENT_PRODUCT_DIRECTORY}" STREQUAL "")
        set(ADD_DOCUMENT_PRODUCT_DIRECTORY "product")
    endif()
    get_filename_component(product_directory ${CMAKE_BINARY_DIR}/${ADD_DOCUMENT_PRODUCT_DIRECTORY} ABSOLUTE)
    # get_filename_component(absolute_product_path ${product_directory}/${target_name} ABSOLUTE)

    ## get primary source
    set(build_sources)
    foreach(input_file ${ADD_DOCUMENT_SOURCES} )
        pandocology_add_input_file(${CMAKE_CURRENT_SOURCE_DIR}/${input_file} ${CMAKE_CURRENT_BINARY_DIR} build_sources)
    endforeach()

    ## get resource files
    set(build_resources)
    foreach(resource_file ${ADD_DOCUMENT_RESOURCE_FILES})
        pandocology_add_input_file(${CMAKE_CURRENT_SOURCE_DIR}/${resource_file} ${CMAKE_CURRENT_BINARY_DIR} build_resources)
    endforeach()

    ## get resource dirs
    set(exported_resources)
    foreach(resource_dir ${ADD_DOCUMENT_RESOURCE_DIRS})
        pandocology_add_input_dir(${CMAKE_CURRENT_SOURCE_DIR}/${resource_dir} ${CMAKE_CURRENT_BINARY_DIR} build_resources)
        if (${ADD_DOCUMENT_EXPORT_ARCHIVE})
            pandocology_add_input_dir(${CMAKE_CURRENT_SOURCE_DIR}/${resource_dir} ${product_directory} exported_resources)
        endif()
    endforeach()

    ## primary command
    if (${ADD_DOCUMENT_DIRECT_TEX_TO_PDF})
        if (${ADD_DOCUMENT_VERBOSE})
            add_custom_command(
                OUTPUT  ${target_name} # note that this is in the build directory
                DEPENDS ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
                # WORKING_DIRECTORY ${working_directory}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${product_directory}
                # we produce the target in the source directory, in case other build targets require it as a source
                COMMAND latexmk -gg -halt-on-error -interaction=nonstopmode -file-line-error -pdf ${build_sources}
                )
        else()
            add_custom_command(
                OUTPUT  ${target_name} # note that this is in the build directory
                DEPENDS ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
                # WORKING_DIRECTORY ${working_directory}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${product_directory}
                # we produce the target in the source directory, in case other build targets require it as a source
                COMMAND latexmk -gg -halt-on-error -interaction=nonstopmode -file-line-error -pdf ${build_sources} 2>/dev/null >/dev/null || (grep --no-messages -A8 ".*:[0-9]*:.*" ${target_stemname}.log && false)
                )
        endif()
        add_to_make_clean(${CMAKE_CURRENT_BINARY_DIR}/${target_name})
    else()
        add_custom_command(
            OUTPUT  ${target_name} # note that this is in the build directory
            DEPENDS ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
            # WORKING_DIRECTORY ${working_directory}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${product_directory}
            # we produce the target in the source directory, in case other build targets require it as a source
            COMMAND ${PANDOC_EXECUTABLE} ${build_sources} ${ADD_DOCUMENT_PANDOC_DIRECTIVES} -o ${target_name}
            )
        add_to_make_clean(${CMAKE_CURRENT_BINARY_DIR}/${target_name})
    endif()

    ## figure out what all is going to be produced by this build set, and set
    ## those as dependencies of the primary target
    set(primary_target_dependencies)
    set(primary_target_dependencies ${primary_target_dependencies} ${CMAKE_CURRENT_BINARY_DIR}/${target_name})
    if (NOT ${ADD_DOCUMENT_NO_EXPORT_PRODUCT})
        set(primary_target_dependencies ${primary_target_dependencies} ${product_directory}/${target_name})
    endif()
    if (${ADD_DOCUMENT_EXPORT_PDF})
        set(primary_target_dependencies ${primary_target_dependencies} ${CMAKE_CURRENT_BINARY_DIR}/${target_stemname}.pdf)
        set(primary_target_dependencies ${primary_target_dependencies} ${product_directory}/${target_stemname}.pdf)
    endif()
    if (${ADD_DOCUMENT_EXPORT_ARCHIVE})
        set(primary_target_dependencies ${primary_target_dependencies} ${product_directory}/${target_stemname}.tbz)
    endif()

    ## primary target
    # # target cannot have same (absolute name) as dependencies:
    # # http://www.cmake.org/pipermail/cmake/2011-March/043378.html
    add_custom_target(
        ${target_name}
        ALL
        DEPENDS ${primary_target_dependencies} ${ADD_DOCUMENT_DEPENDS}
        )

    # run post-pdf
    if (${ADD_DOCUMENT_EXPORT_PDF})
        # get_filename_component(target_stemname ${target_name} NAME_WE)
        add_custom_command(
            OUTPUT ${product_directory}/${target_stemname}.pdf ${CMAKE_CURRENT_BINARY_DIR}/${target_stemname}.pdf
            DEPENDS ${target_name} ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}

            # Does not work: custom template used to generate tex is ignored
            # COMMAND ${PANDOC_EXECUTABLE} ${target_name} -f latex -o ${target_stemname}.pdf

            # (1)   Apparently, both nonstopmode and batchmode produce an output file
            #       even if there was an error. This tricks latexmk into believing
            #       the file is actually up-to-date.
            #       So we use `-halt-on-error` or `-interaction=errorstopmode`
            #       instead.
            # (2)   `grep` returns a non-zero error code if the pattern is not
            #       found. So, in our scheme below to filter the output of
            #       `pdflatex`, it is precisely when there is NO error that
            #       grep returns a non-zero code, which fools CMake into thinking
            #       tex'ing failed.
            #       Hence the need for `| grep ...| cat` or `| grep  || true`.
            #       But we can go better:
            #           latexmk .. || (grep .. && false)
            #       So we can have our cake and eat it too: here we want to
            #       re-raise the error after a successful grep if there was an
            #       error in `latexmk`.
            # COMMAND latexmk -gg -halt-on-error -interaction=nonstopmode -file-line-error -pdf ${target_name} 2>&1 | grep -A8 ".*:[0-9]*:.*" || true
            COMMAND latexmk -gg -halt-on-error -interaction=nonstopmode -file-line-error -pdf ${target_name} 2>/dev/null >/dev/null || (grep --no-messages -A8 ".*:[0-9]*:.*" ${target_stemname}.log && false)

            COMMAND ${CMAKE_COMMAND} -E copy ${target_stemname}.pdf ${product_directory}
            )
        add_to_make_clean(${CMAKE_CURRENT_BINARY_DIR}/${target_stemname}.pdf)
        add_to_make_clean(${product_directory}/${target_stemname}.pdf)
    endif()

    ## copy products
    if (NOT ${ADD_DOCUMENT_NO_EXPORT_PRODUCT})
        add_custom_command(
            OUTPUT ${product_directory}/${target_name}
            DEPENDS ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
            COMMAND ${CMAKE_COMMAND} -E copy ${target_name} ${product_directory}
            )
        add_to_make_clean(${product_directory}/${target_name})
    endif()

    ## copy resources
    if (${ADD_DOCUMENT_EXPORT_ARCHIVE})
        # get_filename_component(target_stemname ${target_name} NAME_WE)
        # add_custom_command(
        #     TARGET ${target_name} POST_BUILD
        #     DEPENDS ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
        #     # COMMAND cp ${build_resources} ${ADD_DOCUMENT_DEPENDS} ${product_directory}
        #     COMMAND ${CMAKE_COMMAND} -E tar cjf ${product_directory}/${target_stemname}.tbz ${target_name} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
        #     )
        add_custom_command(
            OUTPUT ${product_directory}/${target_stemname}.tbz
            DEPENDS ${target_name}
            # COMMAND cp ${build_resources} ${ADD_DOCUMENT_DEPENDS} ${product_directory}
            COMMAND ${CMAKE_COMMAND} -E tar cjf ${product_directory}/${target_stemname}.tbz ${target_name} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
            )
        add_to_make_clean(${product_directory}/${target_stemname}.tbz)
        # add_custom_target(
        #     ${target_stemname}.ARCHIVE
        #     ALL
        #     DEPENDS ${product_directory}/${target_stemname}.tbz
        #     )
    endif()

endfunction(add_document)

function(add_tex_document)
    add_document(${ARGV} DIRECT_TEX_TO_PDF)
endfunction()

# LEGACY SUPPORT
function(add_pandoc_document)
    add_document(${ARGV})
endfunction()

