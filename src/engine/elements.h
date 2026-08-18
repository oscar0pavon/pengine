#ifndef PELEMENT
#define PELEMENT

#include "engine/types.h"
#include <engine/renderer/material.h>

typedef struct Element{
    unsigned int id;
    char name[20];
    bool proccess;
    bool selected;
    Array components;
    TransformComponent* transform;
}Element;

void pe_element_set_material(PMaterial material);

void pe_element_set_scale(vec3 translation);

void pe_element_duplicate(int current_count, Element *original);

void duplicate_selected_element(int current_count, Element* original);

void rotate_element(Element* element, versor quaternion);

void set_element_position(Element* element, vec3 position);

void new_empty_element();

void pe_element_set_position(Element* element,vec3 position);

void set_selected_element_transform(vec3 position, versor rotation);

void select_last_element();

int add_element_with_model_path(const char* model_gltf_path);

#endif
