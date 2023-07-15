#!/bin/bash

shader_dir="$HOME/CLionProjects/fair_engine/engine/shader"

glslc engine/shader/shader.vert -o engine/shader/shader.vert.spv
echo "finished compiling shader.vert"


glslc engine/shader/shader.frag -o engine/shader/shader.frag.spv
echo "finished compiling shader.frag"
