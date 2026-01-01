set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(UNIX)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall>)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Werror>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-pedantic>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wextra>)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Weverything>)
endif()

# disable certain warnings that get emitted by -Weverything
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-padded>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-switch-default>)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-switch-enum>)
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-header-hygiene>)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-weak-vtables>)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-global-constructors>)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-exit-time-destructors>)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wno-c++98-compat-pedantic>)
endif()
endif()

if(NOT YANTRACC)
  set(YANTRACC "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/ycc")
endif()

FUNCTION(ADD_YANTRA)
  CMAKE_PARSE_ARGUMENTS("YCC" "SKIP_GENLINES" "" "SRCS" ${ARGN})

  set(GENLINES "")
  if(YCC_SKIP_GENLINES)
    set(GENLINES "-r")
  endif()

  FOREACH(SRC ${YCC_SRCS})
    GET_FILENAME_COMPONENT(FNAME "${SRC}" NAME_WLE)

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FNAME}.cpp"
      COMMAND "${YANTRACC}"
        -d "${CMAKE_CURRENT_BINARY_DIR}"
        -a
        -f "${SRC}"
        ${GENLINES}
      COMMENT "Compiling ${SRC}"
      DEPENDS "${SRC}" "${YANTRACC}"
    )
  ENDFOREACH()
ENDFUNCTION()
