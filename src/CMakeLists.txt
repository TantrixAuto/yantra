if(UNIX)
add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-ferror-limit=0>)
endif()

# the following custom commands invokes stringify.cmake
# It reads a source .cpp file and generates
# another cpp file, comtaining the source file as a raw string array
add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cb_prototype.cpp"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/cb_prototype.cpp"
    COMMAND ${CMAKE_COMMAND}
        -DPROTOTYPE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/prototype.cpp"
        -DCODEBLOCK_FILE="${CMAKE_CURRENT_BINARY_DIR}/cb_prototype.cpp"
        -DVARNAME="cbPrototype"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/stringify.cmake
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/prototype.cpp" "cpp_generator.cpp"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_utf8.cpp"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_utf8.cpp"
    COMMAND ${CMAKE_COMMAND}
        -DPROTOTYPE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/encoding_utf8.cpp"
        -DCODEBLOCK_FILE="${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_utf8.cpp"
        -DVARNAME="utf8Encoding"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/stringify.cmake
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/encoding_utf8.cpp" "cpp_generator.cpp"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_ascii.cpp"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_ascii.cpp"
    COMMAND ${CMAKE_COMMAND}
        -DPROTOTYPE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/encoding_ascii.cpp"
        -DCODEBLOCK_FILE="${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_ascii.cpp"
        -DVARNAME="asciiEncoding"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/stringify.cmake
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/encoding_ascii.cpp" "cpp_generator.cpp"
)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cb_stream.cpp"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${CMAKE_CURRENT_BINARY_DIR}/cb_stream.cpp"
    COMMAND ${CMAKE_COMMAND}
        -DPROTOTYPE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/stream.hpp"
        -DCODEBLOCK_FILE="${CMAKE_CURRENT_BINARY_DIR}/cb_stream.cpp"
        -DVARNAME="cbStream"
        -P ${CMAKE_CURRENT_SOURCE_DIR}/stringify.cmake
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/stream.hpp" "cpp_generator.cpp"
)

add_executable(ycc
    "${CMAKE_CURRENT_BINARY_DIR}/cb_prototype.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_utf8.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_encoding_ascii.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/cb_stream.cpp"
    "pch.hpp"
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
    "grammar.hpp"
    "grammar.cpp"
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
