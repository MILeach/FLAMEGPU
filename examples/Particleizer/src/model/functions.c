
/*
 * FLAME GPU v 1.5.X for CUDA 9
 * Copyright University of Sheffield.
 * Original Author: Dr Paul Richmond (user contributions tracked on https://github.com/FLAMEGPU/FLAMEGPU)
 * Contact: p.richmond@sheffield.ac.uk (http://www.paulrichmond.staff.shef.ac.uk)
 *
 * University of Sheffield retain all intellectual property and
 * proprietary rights in and to this software and related documentation.
 * Any use, reproduction, disclosure, or distribution of this software
 * and related documentation without an express license agreement from
 * University of Sheffield is strictly prohibited.
 *
 * For terms of licence agreement please attached licence or view licence
 * on www.flamegpu.com website.
 *
 */


#ifndef _FLAMEGPU_FUNCTIONS
#define _FLAMEGPU_FUNCTIONS

#include <header.h>

#define dot(a,b) (a.x*b.x + a.y*b.y + a.z*b.z)


/**
 * output_tri_geometry FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structure of type xmachine_memory_tri. This represents a single agent instance and can be modified directly.
 * @param tri_geometry_message_messages Pointer to output message list of type xmachine_message_tri_geometry_message_list. Must be passed as an argument to the add_tri_geometry_message_message function.
 */
__FLAME_GPU_FUNC__ int output_tri_geometry(xmachine_memory_tri* agent, xmachine_message_tri_geometry_message_list* tri_geometry_message_messages){
    
    
    add_tri_geometry_message_message(tri_geometry_message_messages, agent->id, agent->x, agent->y, agent->z, agent->x1, agent->y1, agent->z1, agent->x2, agent->y2, agent->z2, agent->x3, agent->y3, agent->z3);
       
    return 0;
}

/**
 * test_intersections FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structure of type xmachine_memory_ray. This represents a single agent instance and can be modified directly.
 * @param tri_geometry_message_messages  tri_geometry_message_messages Pointer to input message list of type xmachine_message__list. Must be passed as an argument to the get_first_tri_geometry_message_message and get_next_tri_geometry_message_message functions.* @param intersection_message_messages Pointer to output message list of type xmachine_message_intersection_message_list. Must be passed as an argument to the add_intersection_message_message function.
 */
__FLAME_GPU_FUNC__ int test_intersections(xmachine_memory_ray* agent, xmachine_message_tri_geometry_message_list* tri_geometry_message_messages, xmachine_message_intersection_message_list* intersection_message_messages){
    
 
	// For each triangle
    xmachine_message_tri_geometry_message* current_message = get_first_tri_geometry_message_message(tri_geometry_message_messages);
    while (current_message)
    {
		// Extract triangle vertices
		glm::vec3 v1 = glm::vec3(current_message->x1, current_message->y1, current_message->z1);
		glm::vec3 v2 = glm::vec3(current_message->x2, current_message->y2, current_message->z2);
		glm::vec3 v3 = glm::vec3(current_message->x3, current_message->y3, current_message->z3);
		
		// Test if ray intersects plane triangle lies in
		// Plane defined by (p-p0).n = 0
		// p0 can be any point in the triangle, e.g. any of the vertices, we choose as v1
		// Also need n, given by cross product of two edges of triangle, i.e. v1->v2, v1->v3 then normalized

		glm::vec3 p0 = v1;
		glm::vec3 v1v2 = v2 - v1;
		glm::vec3 v1v3 = v3 - v1;
		glm::vec3 n = glm::vec3(v1v2.y*v1v3.z - v1v2.z*v1v3.y, v1v2.z*v1v3.x - v1v2.x*v1v3.z, v1v2.x*v1v3.y - v1v2.y*v1v3.x);
		float ndotn = dot(n,n);
		n = (1.0f/sqrt(ndotn))*n;

		// Equation of ray is p = r0 + rd
		// Sub into plane equation
		// => ((r0 + rd) - p0).n = 0
		// => (r0 - p0).n + d(r.n) = 0
		// => d = (p0-r0).n / r.n

		// Rays have origin (x, y, 0.5) and project into the screen
		// TODO: Compute to be based on bounding box of object
		glm::vec3 r0 = glm::vec3(agent->x, agent->y, 0.5f);
		glm::vec3 r = glm::vec3(0.0f, 0.0f, -1.0f);

		glm::vec3 r0p0 = p0 - r0;
		float d = dot(r0p0, n) / dot(r, n);

		// If d = 0 ray and plane are parallel -> ignore
		// Otherwise, we have an intersection at r0 + dr
		if (d != 0.0f) {
			glm::vec3 intersection = r0 + d * r;
			printf("Intersection at %f %f %f \n", intersection.x, intersection.y, intersection.z);
			add_intersection_message_message(intersection_message_messages, agent->id, intersection.x, intersection.y, intersection.z);
		}

        
        current_message = get_next_tri_geometry_message_message(current_message, tri_geometry_message_messages);
    }
    
    
  
    
    
    return 0;
}

  


#endif //_FLAMEGPU_FUNCTIONS
