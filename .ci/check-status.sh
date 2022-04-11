#!/usr/bin/env bash

function do_check_format() {
    SOURCES=$(find $(git rev-parse --show-toplevel) | egrep "\.(cpp|cc|c|h)\$")

    CLANG_FORMAT=$(which clang-format-11)
    if [ $? -ne 0 ]; then
        CLANG_FORMAT=$(which clang-format)
        if [ $? -ne 0 ]; then
            echo "[!] clang-format not installed. Unable to check source file format policy." >&2
            exit 1
        fi
    fi

    set -x

    for file in ${SOURCES}; do
        $CLANG_FORMAT ${file} >expected-format
        diff -u -p --label="${file}" --label="expected coding style" ${file} expected-format
    done
    exit $($CLANG_FORMAT --output-replacements-xml ${SOURCES} | egrep -c "</replacement>")
}

function do_cppcheck() {
    local SOURCES=$(find $(git rev-parse --show-toplevel) | egrep "\.(cpp|cc|c|h)\$")

    local CPPCHECK=$(which cppcheck)
    if [ $? -ne 0 ]; then
        echo "[!] cppcheck not installed. Failed to run static analysis the source code." >&2
        exit 1
    fi

    ## Suppression list ##
    # This list will explain the detail of suppressed warnings.
    # The prototype of the item should be like:
    # "- [{file}] {spec}: {reason}"
    #
    # - [hello-1.c] unusedFunction: False positive of init_module and cleanup_module.
    # - [*.c] missingIncludeSystem: Focus on the example code, not the kernel headers.

    local OPTS="
            --enable=warning,style,performance,information
            --suppress=unusedFunction:hello-1.c
            --suppress=missingIncludeSystem
            --std=c11 "

    $CPPCHECK $OPTS --xml ${SOURCES} 2>cppcheck.xml
    local ERROR_COUNT=$(cat cppcheck.xml | egrep -c "</error>")

    if [ $ERROR_COUNT -gt 0 ]; then
        echo "Cppcheck failed: $ERROR_COUNT error(s)"
        cat cppcheck.xml
        exit 1
    fi
}

function do_gcc()
{
    local GCC=$(which gcc-10)
    if [ $? -ne 0 ]; then
        echo "[!] gcc-10 is not installed. Failed to run static analysis with GCC." >&2
        exit 1
    fi

    make -C examples CONFIG_STATUS_CHECK_GCC=y STATUS_CHECK_GCC=$GCC 2> gcc.log

    local WARNING_COUNT=$(cat gcc.log | egrep -c " warning:" )
    local ERROR_COUNT=$(cat gcc.log | egrep -c " error:" )
    local COUNT=`expr $WARNING_COUNT + $ERROR_COUNT`
    if [ $COUNT -gt 0 ]; then
        echo "gcc failed: $WARNING_COUNT warning(s), $ERROR_COUNT error(s)"
        cat gcc.log
        exit 1
    fi
    make -C examples CONFIG_STATUS_CHECK_GCC=y STATUS_CHECK_GCC=$GCC clean
}

do_check_format
do_cppcheck
do_gcc

exit 0
