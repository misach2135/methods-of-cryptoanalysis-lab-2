#!/bin/bash

cmake -B ./out -S . -G Ninja
ninja -C ./out