#ifndef ENGINE_COMPONENTS_H
#define ENGINE_COMPONENTS_H

typedef struct Element Element;
#include <engine/array.h>
#include <engine/types.h>
#include <cglm/cglm.h>
#include "static_mesh_component.h"
#include <engine/model.h>

typedef enum ComponentType{
    TRASNFORM_COMPONENT = 0,
		PE_COMP_PLAYER_START,
    SPHERE_COMPONENT,
    CAMERA_COMPONENT,
    CUBE_COMPONENT,
    STATIC_MESH_COMPONENT,
    COMPONENT_SKINNED_MESH,
    LEVEL_OF_DETAIL_COMPONENT,
    COMPONENT_HLOD,
    COMPONENT_HLOD_BOX,
}ComponentType;

typedef struct PEComponentPlayerStart{
	vec3 position;
}PEComponentPlayerStart;

typedef struct ComponentDefinition{
    ComponentType type;
    Element* parent;
    unsigned int id;
    unsigned short int bytes_size;
    void* data;
}ComponentDefinition;

typedef struct SphereComponent{    
    int radius;
    vec3 center;
    PModel* model;
}SphereComponent;

typedef struct CubeComponent{    
    vec3 center;
    PModel* model;
}CubeComponent;

typedef struct TransformComponent{    
    vec3 position;
    vec4 rotation;
    vec3 scale;
    mat4 model_matrix;
}TransformComponent;

typedef struct CameraComponent{
    mat4 projection;
    mat4 view;
    vec3 front;
    vec3 up;
    vec3 position;
    PModel* camera_gizmo;
}CameraComponent;

typedef struct HierarchicalLevelOfDetail HierarchicalLevelOfDetail;

typedef struct HierarchicalLevelOfDetail{
    PModel model;
    float distance;
    bool draw;
    bool drew;
    HierarchicalLevelOfDetail* parent;
    Array childs;
    vec3 center;
    vec3 bounding_box[2];    
}HierarchicalLevelOfDetail;

typedef struct HLODComponent{
    vec3 center;
    vec3 bounding_box[2];
    PModel* model;
    float distance;
    bool has_childs_HLOD;
    Array childs;//HLOD childs
    
}HLODComponent;

typedef struct HLODBoxComponent{
    vec3 center;
    vec3 bounding_box[2];     
}HLODBoxComponent;


typedef struct LevelOfDetailComponent{
    Array meshes;
    Array distances;
    int texture_id;
    int model_id;
    bool initialized;
    HierarchicalLevelOfDetail* hirarchical_level_of_detail;
}LevelOfDetailComponent;

/*Add components to selected element. The u32 is the models loaded*/
void pe_comp_add(u32);

void add_component_to_selected_element(int component_byte_size, void* new_component, ComponentType type);
void init_sphere_component(SphereComponent* component);
void init_camera_component(CameraComponent* component);
void init_cube_component(CubeComponent* component);
void update_per_frame_component(ComponentDefinition*);
void update_component(ComponentDefinition*);
void init_transfrom_component(TransformComponent* component);
void* pe_comp_get(ComponentType type);
void* get_component_from_element(Element* element, ComponentType type);
void change_view_to_camera_component(CameraComponent* camera_component);
void add_transform_component_to_selected_element();
void add_camera_component_to_selected_element();
void update_main_camera_with_camera_component_values(CameraComponent* camera_component);
void clean_component_value(ComponentDefinition* component);
void init_element_component(ComponentDefinition* element_component);
void for_each_element_components(void(*do_to)(ComponentDefinition*));

void pe_element_comp_init();

void component_add_HLOD_to_select_element();

void pe_mesh_fill_models_ids(Array *meshes, Array *textures,
                             u32 models_loaded);
void for_each_element_components_in_array_of_pp(Array* array, void(*do_to)(ComponentDefinition*));
#endif
