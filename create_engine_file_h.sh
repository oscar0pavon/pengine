#!/bin/sh

WORKDIR=$(pwd)

ENGINE_FILES="char file_std_vert[] = \"${WORKDIR}/shaders/std.vert\"; "

ENGINE_FILES+="char file_diffuse_frag[] = \"${WORKDIR}/shaders/diffuse.frag\"; "

ENGINE_FILES+="char file_skin_vertex_shader[] = \"${WORKDIR}/shaders/skin_vertex_shader.frag\"; "


if [ -e ${WORKDIR}/src/engine/files.h ]
then
    rm -f ${WORKDIR}/src/engine/files.h
fi

echo ${ENGINE_FILES} > ${WORKDIR}/src/engine/files.h




