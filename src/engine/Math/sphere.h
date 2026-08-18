#ifndef ENGINE_MATH_SPHERE_H
#define ENGINE_MATH_SPHERE_H

#include <cglm/cglm.h>
#include "math.h"

typedef struct Sphere{
    vec3 center;
    float radius;
    float volume;
}Sphere;

inline static bool sphere_inside_sphere(Sphere* sphere01, Sphere* sphere02){

    float distance_cglm = glm_vec3_distance(sphere01->center,sphere02->center);
    
    if( (distance_cglm+sphere01->radius)<= sphere02->radius)
        return true;
    return false;
}

inline static bool sphere_intersect_with_sphere(Sphere* sphere01, Sphere* sphere02){
    vec3 distance;
    glm_vec3_sub(sphere02->center,sphere01->center,distance);
    float easy_distance = glm_vec3_dot(distance,distance);

    float sum_radius_squared = sphere01->radius + sphere02->radius;
    sum_radius_squared *= sum_radius_squared;
    
    if(easy_distance <= sum_radius_squared)
        return true;

    return false;
}

inline static float get_sphere_volume(Sphere* sphere){
    
    float volume = ( (4.f/3.f) * M_PI * cube(sphere->radius) );
    sphere->volume = volume;
    return volume;
    
}

inline static void sphere_merge(Sphere* sphere01, Sphere* sphere02, Sphere* out){
    if(sphere_inside_sphere(sphere02, sphere01)){
        glm_vec3_copy(sphere01->center,out->center);       
        out->radius = sphere01->radius;
    }else if( sphere_inside_sphere(sphere01, sphere02) )
    {
        out->radius = sphere02->radius;
        glm_vec3_copy(sphere02->center,out->center);

    }else{
       /*  New radius
        R = (r1 + r2 + |c1 - c2|) / 2 */
        float new_radius = sphere01->radius + sphere02->radius;
        vec3 center;
        glm_vec3_sub(sphere01->center,sphere02->center,center);
        float center_magnitude = sqrt(center[0] * center[0] + center[1] * center[1] + center[2] * center[2]);
        new_radius = (new_radius+center_magnitude) * 0.5;
        out->radius = new_radius;

       /*  New center*/
        glm_vec3_center(sphere01->center,sphere02->center,out->center);
        
    }   
    

}

inline static float sphere_get_height_cap(Sphere* sphere, float distance){
    float height_sphere01 = (sphere->radius * sphere->radius ) - ( (sphere->radius - distance) * (sphere->radius - distance) );
    return height_sphere01 / (2 * distance);
}

inline static bool sphere_equals(Sphere* sphere, Sphere* sphere2){
    if (glm_vec3_eqv(sphere2->center,sphere->center) )
    {
        if(sphere->radius == sphere2->radius){
            return true;
        }
    }
    return false;
}

#endif
