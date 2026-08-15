include_guard(GLOBAL)

function(download_userver)
    set(options)
    set(one_value_args TRY_DIR VERSION GIT_TAG)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "" ${ARGN})

    if(ARG_TRY_DIR)
        get_filename_component(ARG_TRY_DIR "${ARG_TRY_DIR}" REALPATH)
        if(EXISTS "${ARG_TRY_DIR}")
            message(STATUS "Using userver from ${ARG_TRY_DIR}")
            add_subdirectory("${ARG_TRY_DIR}" third_party/userver)
            return()
        endif()
    endif()

    include(get_cpm)
    if(NOT DEFINED CPM_USE_NAMED_CACHE_DIRECTORIES)
        set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
    endif()

    cpmaddpackage(
        NAME userver
        GITHUB_REPOSITORY userver-framework/userver
        VERSION ${ARG_VERSION}
        GIT_TAG ${ARG_GIT_TAG}
        ${ARG_UNPARSED_ARGUMENTS}
    )
endfunction()
