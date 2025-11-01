# this file has the following arguments:
# ${PROTOTYPE_FILE} : this is the input file (typically XXX.cpp or XXX.hpp)
# ${CODEBLOCK_FILE} : this is the output file (typically cb_XXX.cpp)
# ${VARNAME}        : variable name

function(FlushChunk)
    get_property(tCHUNK GLOBAL PROPERTY CHUNK)
    file(APPEND ${CODEBLOCK_FILE} "${tCHUNK}")
endfunction()

function(WriteLine DATA)
    get_property(tCHUNK GLOBAL PROPERTY CHUNK)
    string(CONCAT tCHUNK "${tCHUNK}" "${DATA}\n")
    set_property(GLOBAL PROPERTY CHUNK "${tCHUNK}")

    string(LENGTH "${tCHUNK}" CHUNK_SIZE)
    if( ${CHUNK_SIZE} GREATER 1500)
        FlushChunk()
        set(tCHUNK "")
        set_property(GLOBAL PROPERTY CHUNK "${tCHUNK}")
    endif()
endfunction()

message("PROTOTYPE_FILE: ${PROTOTYPE_FILE}")

set(BLOCK_SIZE 0)

file(READ "${PROTOTYPE_FILE}" file_content)

string(LENGTH "${file_content}" length)

set(LINE "")

math(EXPR index 0)
WriteLine("extern const char* ${VARNAME};")
WriteLine("const char* ${VARNAME} = R\"ENDTABLES(")
while(index LESS length)
    string(SUBSTRING "${file_content}" ${index} 1 char)
    if(char STREQUAL "\n")
        WriteLine("${LINE}")
        math(EXPR BLOCK_SIZE "${BLOCK_SIZE} + 1" )
        message("bs: ${BLOCK_SIZE}")
        if( ${BLOCK_SIZE} GREATER 50)
            WriteLine(")ENDTABLES\"")
            WriteLine("R\"ENDTABLES(")
            set(BLOCK_SIZE 0)
        endif()
        set(LINE "")
    else()
        string(APPEND LINE "${char}")
    endif()
    math(EXPR index "${index} + 1")
endwhile()

WriteLine(")ENDTABLES\"; //${CBID}\n")
FlushChunk()
