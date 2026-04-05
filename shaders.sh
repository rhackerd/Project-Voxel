#!/bin/bash

SHADER_DIR="shaders"
OUT_DIR="assets/shaders"

mkdir -p "$OUT_DIR"

for slang_file in "$SHADER_DIR"/*.slang; do
    name=$(basename "$slang_file" .slang)
    echo "Compiling $name..."
    
    slangc "$slang_file" -entry vertMain -stage vertex   -target spirv -o "$OUT_DIR/$name.vert.spv"
    slangc "$slang_file" -entry fragMain -stage fragment -target spirv -o "$OUT_DIR/$name.frag.spv"
done

echo "Done"