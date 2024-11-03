#!/bin/sh

WORKDIR=$(pwd)

FILE=${WORKDIR}/src/engine/files.h
touch ${FILE}


echo "#ifndef _FILES_H_" > ${FILE}
echo "#define _FILES_H_" >> ${FILE}


echo "#define file_std_vert \"${WORKDIR}/src/shaders/std.vert\"" >> ${FILE}
echo "#define file_diffuse_frag \"${WORKDIR}/src/shaders/diffuse.frag\"" >> ${FILE}


echo "#define file_skin_vertex_shader \"${WORKDIR}/src/shaders/skin_vertex_shader.frag\"" >> ${FILE}


echo "#define file_vertex_modeling \"${WORKDIR}/src/shaders/vertex_modelling.frag\"" >> ${FILE}

#editor gizmos
echo "#define file_translate_glb \"${WORKDIR}/content/editor/translate.glb\"" >> ${FILE}
echo "#define file_rotate_glb \"${WORKDIR}/content/editor/rotate.glb\"" >> ${FILE}
echo "#define file_scale_glb \"${WORKDIR}/content/editor/scale.glb\"" >> ${FILE}
echo "#define file_camera_gltf \"${WORKDIR}/content/editor/camera_gltf\"" >> ${FILE}
echo "#define file_player_start_glb \"${WORKDIR}/content/editor/player_start.glb\"" >> ${FILE}


echo "#define file_transform_jpg \"${WORKDIR}/content/editor/transform_gizmo.jpg\"" >> ${FILE}
echo "#define file_rotate_png \"${WORKDIR}/content/editor/rotate_gizmo.png\"" >> ${FILE}
echo "#define file_camera_jpg \"${WORKDIR}/content/editor/camera_gizmo.jpg\"" >> ${FILE}
echo "#define file_player_start_jpg \"${WORKDIR}/content/editor/player_start_gizmo.jpg\"" >> ${FILE}

echo "#endif" >> ${FILE}





