if(UNIX)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ferror-limit=0>)
    endif()
endif()

FUNCTION(STRINGIFY)
  FOREACH(SRC ${ARGV})
    GET_FILENAME_COMPONENT(FNAME "${SRC}" NAME_WLE)

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cb_${FNAME}.cpp"
        COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/cb_${FNAME}.cpp"
        COMMAND ${CMAKE_COMMAND}
            -DPROTOTYPE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/${SRC}"
            -DCODEBLOCK_FILE="${CMAKE_CURRENT_BINARY_DIR}/cb_${FNAME}.cpp"
            -DVARNAME="cb_${FNAME}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/stringify.cmake
        DEPENDS "${SRC}" "cpp_generator.cpp"
    )
  ENDFOREACH()
ENDFUNCTION()

STRINGIFY(
    "prototype.cpp"
    "encoding_utf8.cpp"
    "encoding_ascii.cpp"
    "stream.hpp"
    "text_writer.hpp"
    "print.hpp"
    "nsutil.hpp"
    "filepos.hpp"
)

configure_file(config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_executable(ycc
    "${CMAKE_CURRENT_BINARY_DIR}/cb_prototype.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_utf8.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_ascii.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_stream.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_text_writer.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_print.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_nsutil.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_filepos.cpp"
    "pch.hpp"
    "print.hpp"
    "nsutil.hpp"
    "util.hpp"
    "error.hpp"
    "logger.hpp"
    "codeblock.hpp"
    "filepos.hpp"
    "stream.hpp"
    "parser.hpp"
    "parser.cpp"
    "lexer_builder.hpp"
    "lexer_builder.cpp"
    "parser_builder.hpp"
    "parser_builder.cpp"
    "encodings.hpp"
    "encoding_utf8.cpp"
    "encoding_ascii.cpp"
    "grammar_yglx.hpp"
    "grammar_ygp.hpp"
    "grammar_yg.hpp"
    "grammar_ygp.cpp"
    "grammar_yglx.cpp"
    "text_writer.hpp"
    "tx_table.hpp"
    "grammar_printer.hpp"
    "grammar_printer.cpp"
    "cpp_generator.hpp"
    "cpp_generator.cpp"
    "main.cpp"
)
target_include_directories(ycc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(proto
    "prototype.cpp"
)
