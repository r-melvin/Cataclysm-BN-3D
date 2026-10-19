#!/usr/bin/env bash
set -euo pipefail

shadercross="${SHADERCROSS:-shadercross}"
source_dir="${1:-src/shaders}"
output_dir="${2:-data/shaders}"

has_shadercross=0
can_compile_hlsl=0
if command -v "${shadercross}" >/dev/null 2>&1; then
    has_shadercross=1
    if "${shadercross}" "${source_dir}/test_compute.hlsl" -s hlsl -d spirv -t compute -o /dev/null >/dev/null 2>&1; then
        can_compile_hlsl=1
    fi
fi

has_glslang=0
if command -v glslangValidator >/dev/null 2>&1; then
    has_glslang=1
fi

has_spirvcross=0
if command -v spirv-cross >/dev/null 2>&1; then
    has_spirvcross=1
fi

if [ "${can_compile_hlsl}" -eq 0 ] && [ "${has_glslang}" -eq 0 ]; then
    echo "No suitable HLSL compiler found (shadercross with DXC or glslangValidator required)" >&2
    exit 1
fi

if [ ! -d "${source_dir}" ]; then
    echo "Shader source directory not found: ${source_dir}" >&2
    exit 1
fi

mkdir -p "${output_dir}"
find "${output_dir}" -maxdepth 1 \( -name '*.spv' -o -name '*.msl' -o -name '*.dxil' \) -delete

shopt -s nullglob
for shader in "${source_dir}"/*.hlsl; do
    shader_file="$(basename "${shader}")"
    shader_name="${shader_file%.hlsl}"
    stage="compute"
    glslang_stage="comp"
    if [[ "${shader_name}" == *_vertex ]]; then
        stage="vertex"
        glslang_stage="vert"
    elif [[ "${shader_name}" == *_fragment ]]; then
        stage="fragment"
        glslang_stage="frag"
    fi

    if [ "${can_compile_hlsl}" -eq 1 ]; then
        "${shadercross}" "${shader}" -s hlsl -d spirv -t "${stage}" -o "${output_dir}/${shader_name}.spv"
        "${shadercross}" "${shader}" -s hlsl -d msl -t "${stage}" -o "${output_dir}/${shader_name}.msl"
        "${shadercross}" "${shader}" -s hlsl -d dxil -t "${stage}" -o "${output_dir}/${shader_name}.dxil"
    else
        glslangValidator -D -e main -S "${glslang_stage}" "${shader}" -V -o "${output_dir}/${shader_name}.spv" >/dev/null

        if [ "${has_spirvcross}" -eq 1 ]; then
            spirv-cross --msl "${output_dir}/${shader_name}.spv" --output "${output_dir}/${shader_name}.msl"
        elif [ "${has_shadercross}" -eq 1 ]; then
            "${shadercross}" "${output_dir}/${shader_name}.spv" -s spirv -d msl -t "${stage}" -o "${output_dir}/${shader_name}.msl"
        fi

        if [ "${has_shadercross}" -eq 1 ]; then
            if ! "${shadercross}" "${output_dir}/${shader_name}.spv" -s spirv -d dxil -t "${stage}" -o "${output_dir}/${shader_name}.dxil" 2>/dev/null; then
                rm -f "${output_dir}/${shader_name}.dxil"
            fi
        fi
    fi
done
