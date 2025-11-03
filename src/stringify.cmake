# This file is used to embed a C++ source/header file into another .cpp file
# as a raw string literal.
# It takes the following arguments:
# ${PROTOTYPE_FILE} : this is the input file (typically XXX.cpp or XXX.hpp)
# ${CODEBLOCK_FILE} : this is the output file (typically cb_XXX.cpp)
# ${VARNAME}        : variable name

# msvc has a limit on the size of a raw string literal, so we break it into blocks of 50 lines

# read the input file into `file_content`
file(READ "${PROTOTYPE_FILE}" file_content)

# get the length of `file_content`
string(LENGTH "${file_content}" length)

# initialize all variables
set(BLOCK_SIZE 0)
set(LINE "")
math(EXPR index 0)

# write code to declare the string literal
file(APPEND ${CODEBLOCK_FILE} "extern const char* const ${VARNAME};\n")

# write code to open the string literal
file(APPEND ${CODEBLOCK_FILE} "const char* const ${VARNAME} = R\"ENDTABLES(\n")

# iterate over each character in `file_content`
while(index LESS length)
    string(SUBSTRING "${file_content}" ${index} 1 char)

    # if the character is a newline, append the current line to the output file
    if(char STREQUAL "\n")
        # append the current line to the output file
        file(APPEND ${CODEBLOCK_FILE} "${LINE}\n")
        set(LINE "")

        # increment the block size
        math(EXPR BLOCK_SIZE "${BLOCK_SIZE} + 1" )

        # if 50 lines were written so far, close and reopen the raw string literal
        if( ${BLOCK_SIZE} GREATER 50)
            file(APPEND ${CODEBLOCK_FILE} ")ENDTABLES\"\n")
            file(APPEND ${CODEBLOCK_FILE} "R\"ENDTABLES(\n")
            set(BLOCK_SIZE 0)
        endif()
    else()
        # append the character to the current line
        string(APPEND LINE "${char}")
    endif()

    # increment the index
    math(EXPR index "${index} + 1")
endwhile()

# write code to close the string literal
file(APPEND ${CODEBLOCK_FILE} ")ENDTABLES\"; //${CBID}\n")
