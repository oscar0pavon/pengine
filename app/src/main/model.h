#ifndef MODEL_H
#define MODEL_H

#include "utils.h"
#include "vector.h"
#include <GLES2/gl2.h>

#include <cglm.h>

#include "images.h"

#include "skeletal.h"


typedef struct Model{
    VertexArray vertex_array;
    IndexArray index_array;

    GLuint vertex_buffer_id;
    GLuint index_buffer_id;

    mat4 model_mat;
    GLuint shader;

    Texture texture;

    Skeletal* skeletal;
}Model;

struct Geometry{

};


void init_model();
int load_model(const char* path, struct Model*);


void parse_json(const char* json_file, size_t json_file_size);

#endif // !MODEL_H