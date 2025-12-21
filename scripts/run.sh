#!/usr/bin/bash

silent_run() {
    local err
    err=$("$@" 2>&1 >/dev/null)
    local status=$?
    if (( status != 0 )); then
        echo "❌ $* failed:"
        echo "$err"
        return $status
    fi
}

silent_run cmake -B ./out -S . || exit $?
silent_run make -C ./out       || exit $?

./out/lab2 "$@"