#!/bin/sh

WORKDIR=$(pwd)

FILE=${WORKDIR}/src/engine/files.h
touch ${FILE}


echo "#ifndef _FILES_H_" > ${FILE}
echo "#define _FILES_H_" >> ${FILE}


echo "#define file_std_vert \"${WORKDIR}/src/shaders/std_vert.glsl\"" >> ${FILE}
echo "#define file_diffuse_frag \"${WORKDIR}/src/shaders/diffuse_frag.glsl\"" >> ${FILE}


echo "#define file_skin_vertex_shader \"${WORKDIR}/src/shaders/skin_vertex_shader.glsl\"" >> ${FILE}


echo "#define file_vertex_modeling \"${WORKDIR}/src/shaders/vertex_modelling.glsl\"" >> ${FILE}

#editor gizmos
echo "#define file_translate_glb \"${WORKDIR}/content/editor/translate.glb\"" >> ${FILE}
echo "#define file_rotate_glb \"${WORKDIR}/content/editor/rotate.glb\"" >> ${FILE}
echo "#define file_scale_glb \"${WORKDIR}/content/editor/scale.glb\"" >> ${FILE}
echo "#define file_camera_gltf \"${WORKDIR}/content/editor/camera.gltf\"" >> ${FILE}
echo "#define file_player_start_glb \"${WORKDIR}/content/editor/player_start.gltf\"" >> ${FILE}


echo "#define file_transform_jpg \"${WORKDIR}/content/editor/transform_gizmo.jpg\"" >> ${FILE}
echo "#define file_rotate_png \"${WORKDIR}/content/editor/rotate_gizmo.png\"" >> ${FILE}
echo "#define file_camera_jpg \"${WORKDIR}/content/editor/camera_gizmo.jpg\"" >> ${FILE}
echo "#define file_player_start_jpg \"${WORKDIR}/content/editor/player_start_gizmo.jpg\"" >> ${FILE}


#vulkan shader .spv


echo "#define file_other_vert_spv \"${WORKDIR}/src/shaders/other_vert.spv\"" >> ${FILE}
echo "#define file_blue_frag_spv \"${WORKDIR}/src/shaders/blue_frag.spv\"" >> ${FILE}
echo "#define file_vert_spv \"${WORKDIR}/src/shaders/vert.spv\"" >> ${FILE}
echo "#define file_frag_spv \"${WORKDIR}/src/shaders/frag.spv\"" >> ${FILE}
echo "#define file_in_position_spv \"${WORKDIR}/src/shaders/in_position_vert.spv\"" >> ${FILE}
echo "#define file_diffuse_vert_spv \"${WORKDIR}/src/shaders/diffuse_vert.spv\"" >> ${FILE}
echo "#define file_diffuse_frag_spv \"${WORKDIR}/src/shaders/diffuse_frag.spv\"" >> ${FILE}
echo "#define file_skinned_spv \"${WORKDIR}/src/shaders/skinned.spv\"" >> ${FILE}

#font

echo "#define file_font \"${WORKDIR}/content/DejaVuSerif.ttf\"" >> ${FILE}

#chess


echo "#define file_peon_glb\"${WORKDIR}/demos/chess/peon.glb\"" >> ${FILE}
echo "#define file_reina_glb\"${WORKDIR}/demos/chess/reina.glb\"" >> ${FILE}



echo "#endif" >> ${FILE}





