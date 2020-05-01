
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

/*
	0.25m x 0.5m x 1.0m volume of a fluid in a 1m x 1m x 1m box
	Total fluid volume: 0.125m^3
	Fluid density: 1000kg/m^3
	Total fluid weight: 125kg
	Number of particles: 64,000
	Particle weight: 125kg/64,000 = 0.015625kg
	Initial particle spacing ~= 0.0125m

*/


#ifndef _FLAMEGPU_FUNCTIONS
#define _FLAMEGPU_FUNCTIONS

#include <header.h>

// Mathematic constants
#define PI 3.141593f

// Sim parameters
#define TIMESTEP 0.001f
#define SMOOTHING_LENGTH 0.057f

// Environemental parameters
#define RESTITUTION_COEFFICIENT 0.8f
#define HALF_BOUNDARY_WIDTH 0.5f

// Fluid properties
#define PARTICLE_MASS 0.001953125f
#define PRESSURE_COEFFICIENT 0.5f
#define FLUID_REST_DENSITY 1000.0f
#define MINIMUM_PARTICLE_DENSITY 0.2f
#define MAXIMUM_PARTICLE_DENSITY 1000000.0f
#define VISCOSITY 200.05f

//// Math helpers
inline __device__ float dot(glm::vec3 a, glm::vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline __device__ float length(glm::vec3 v) {
	return sqrt(dot(v, v));
}

//// Smoothing functions
__device__ float computeW(float distance, float smoothingLength) {
	//float relativeDistance = distance / smoothingLength;
	//float w = 0.0f;

	//if (relativeDistance > 2)
	//	w = 0;
	//else if (relativeDistance > 1) {
	//	w = pow(2 - relativeDistance, 3) / 4.0f;
	//}
	//else {
	//	w = 1 - (3.0f / 2.0f)*pow(relativeDistance, 2) + (3.0f / 4.0f)*pow(relativeDistance, 3);
	//}
	//	
	//// Account for 3 dimensions
	//w /= PI;
	//w /= pow(SMOOTHING_LENGTH, 3);
	//return w;


	// Poly6
	return distance < smoothingLength ? (315.0f / (64.0f*PI*pow(smoothingLength, 9))) * pow((pow(smoothingLength, 2) - pow(distance, 2)), 3) : 0.0f;
}

__device__ glm::vec3 computeDelW(float distance, glm::vec3 difference, float smoothingLength) {
	//float relativeDistance = distance / smoothingLength;
	//float w = 0.0f;

	//// Cubic spline
	//if (relativeDistance > 2)
	//	w = 0;
	//else if (relativeDistance > 1) {
	//	w = (-3.0f / 4.0f)*pow((2-relativeDistance),2);
	//}
	//else {
	//	w = -3 * relativeDistance + (9.0f / 4.0f)*pow(relativeDistance, 2);
	//}

	//// Account for 3 dimensions
	//w /= PI;
	//w /= pow(SMOOTHING_LENGTH, 3);


	//// Spiky
	////w = (-45.0f / (PI * pow(smoothingLength, 6))) * pow(1 - relativeDistance, 2);
	//
	//return w * difference;

	return distance < smoothingLength ? -(45.0f / (PI*pow(smoothingLength, 6)))*(pow(smoothingLength - distance, 2))* (1.0f / distance) * difference : glm::vec3(0.0f);
	//return distance < smoothingLength ? -((smoothingLength/distance) - 1.0f)*difference : glm::vec3(0.0f);
}

__device__ float computeDelSqW(float distance, float smoothingLength) {
	/*float w = 0.0f;
	float relativeDistance = distance / smoothingLength;
	
	if (relativeDistance > 1) {
		w = 0.0f;
	}
	else {
		w = (15 / (2 * PI*pow(smoothingLength, 3))) * (-pow(relativeDistance, 3) / 2 + pow(relativeDistance, 2) + relativeDistance / 2 - 1);
	}
	return w;*/

	//return distance < smoothingLength ? (45.0f / (PI*pow(smoothingLength, 6))) * (smoothingLength - distance) : 0.0f;
	return distance < smoothingLength ? 1.0f - distance/smoothingLength : 0.0f;
}

/**
 * outputLocationVelocity FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structure of type xmachine_memory_FluidParticle. This represents a single agent instance and can be modified directly.
 * @param location_velocity_messages Pointer to output message list of type xmachine_message_location_velocity_list. Must be passed as an argument to the add_location_velocity_message function.
 */
__FLAME_GPU_FUNC__ int outputLocationVelocity(xmachine_memory_FluidParticle* agent, xmachine_message_location_velocity_list* location_velocity_messages){
    
    add_location_velocity_message(location_velocity_messages, agent->id, agent->x, agent->y, agent->z, agent->dx, agent->dy, agent->dz);

    return 0;
}

/**
 * computeDensityPressure FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structure of type xmachine_memory_FluidParticle. This represents a single agent instance and can be modified directly.
 * @param location_velocity_messages  location_velocity_messages Pointer to input message list of type xmachine_message__list. Must be passed as an argument to the get_first_location_velocity_message and get_next_location_velocity_message functions.* @param density_pressure_messages Pointer to output message list of type xmachine_message_density_pressure_list. Must be passed as an argument to the add_density_pressure_message function.
 */
__FLAME_GPU_FUNC__ int computeDensityPressure(xmachine_memory_FluidParticle* agent, xmachine_message_location_velocity_list* location_velocity_messages, xmachine_message_location_velocity_PBM* partition_matrix, xmachine_message_density_pressure_list* density_pressure_messages){

	float density = 0.0f;
    
    xmachine_message_location_velocity* current_message = get_first_location_velocity_message(location_velocity_messages, partition_matrix, agent->x, agent->y, agent->z);
	while (current_message)
	{
		// For each agent add weighted density contribution
		glm::vec3 diff = glm::vec3(current_message->x - agent->x, current_message->y - agent->y, current_message->z - agent->z);
		float distance = length(diff);
		density += PARTICLE_MASS*computeW(distance, SMOOTHING_LENGTH);
		
		current_message = get_next_location_velocity_message(current_message, location_velocity_messages, partition_matrix);
	}

	
	agent->density = max(density, MINIMUM_PARTICLE_DENSITY);
	agent->pressure = max(PRESSURE_COEFFICIENT * (agent->density - FLUID_REST_DENSITY), 0.0f);

    add_density_pressure_message(density_pressure_messages, agent->id, agent->density, agent->pressure, agent->x, agent->y, agent->z, agent->dx, agent->dy, agent->dz, agent->isStatic);

    return 0;
}

/**
 * computeForce FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structure of type xmachine_memory_FluidParticle. This represents a single agent instance and can be modified directly.
 * @param density_pressure_messages  density_pressure_messages Pointer to input message list of type xmachine_message__list. Must be passed as an argument to the get_first_density_pressure_message and get_next_density_pressure_message functions.* @param force_messages Pointer to output message list of type xmachine_message_force_list. Must be passed as an argument to the add_force_message function.
 */
__FLAME_GPU_FUNC__ int computeForce(xmachine_memory_FluidParticle* agent, xmachine_message_density_pressure_list* density_pressure_messages, xmachine_message_density_pressure_PBM* partition_matrix, xmachine_message_force_list* force_messages){
    
	glm::vec3 pressure = glm::vec3(0.0f);

    xmachine_message_density_pressure* current_message = get_first_density_pressure_message(density_pressure_messages, partition_matrix, agent->x, agent->y, agent->z);
    while (current_message)
    {
		// For each other agent
		if (agent->id != current_message->id)
		{
			// Add weighted pressure contribution
			glm::vec3 diff = glm::vec3(current_message->x - agent->x, current_message->y - agent->y, current_message->z - agent->z);
			float distance = length(diff);			
			float weight = (agent->pressure + current_message->pressure) / (2.0f * current_message->density);
			glm::vec3 del_w = computeDelW(distance, diff, SMOOTHING_LENGTH);
			pressure += PARTICLE_MASS * weight * del_w;


			if (current_message->isStatic == false)
			{
				// Add viscosity
				glm::vec3 velocityDiff = glm::vec3(current_message->dx - agent->dx, current_message->dy - agent->dy, current_message->dz - agent->dz);
				//glm::vec3 Vij = (VISCOSITY * PARTICLE_MASS / (agent->density * current_message->density)) * velocityDiff;
				glm::vec3 Vij = (VISCOSITY * PARTICLE_MASS) * velocityDiff;
				float laplacian = computeDelSqW(distance, SMOOTHING_LENGTH);

				pressure += laplacian * Vij;
			}

			// Add surface tension
			float st = 0.0f;
			if (distance > SMOOTHING_LENGTH / 2.0f && distance <= SMOOTHING_LENGTH)
				st = 10 * (32.0f / (PI*pow(SMOOTHING_LENGTH, 6))) * pow(SMOOTHING_LENGTH - distance, 3) * pow(distance, 3);
			if (distance > 0 && distance <= SMOOTHING_LENGTH / 2.0f)
				st = ((64.0f / (PI*pow(SMOOTHING_LENGTH, 6))) * 2 * pow(SMOOTHING_LENGTH - distance, 3) * pow(distance, 3)) - pow(SMOOTHING_LENGTH, 4) / 64.0f;
			pressure += st * diff;
		}
        
        current_message = get_next_density_pressure_message(current_message, density_pressure_messages, partition_matrix);
    }

	agent->fx = pressure.x;
	agent->fy = pressure.y;
	agent->fz = pressure.z;
 
    
    return 0;
}

/**
 * integrate FLAMEGPU Agent Function
 * Automatically generated using functions.xslt
 * @param agent Pointer to an agent structure of type xmachine_memory_FluidParticle. This represents a single agent instance and can be modified directly.
 * @param force_messages  force_messages Pointer to input message list of type xmachine_message__list. Must be passed as an argument to the get_first_force_message and get_next_force_message functions.
 */
__FLAME_GPU_FUNC__ int integrate(xmachine_memory_FluidParticle* agent, xmachine_message_force_list* force_messages, xmachine_message_force_PBM* partition_matrix){
    
	// Integrate forces due to pressure and viscosity
	agent->dx += (agent->fx) * TIMESTEP;
	agent->dy += (agent->fy) * TIMESTEP;
	agent->dz += (agent->fz) * TIMESTEP ;

	// Gravity
	agent->dy -= 9.8f * TIMESTEP;

	// Boundary conditions
	if (abs(agent->x + agent->dx*TIMESTEP) > HALF_BOUNDARY_WIDTH)
		agent->dx = -agent->dx*RESTITUTION_COEFFICIENT;
	if (abs(agent->y + agent->dy*TIMESTEP) > HALF_BOUNDARY_WIDTH)
		agent->dy = -agent->dy*RESTITUTION_COEFFICIENT;
	if (abs(agent->z + agent->dz*TIMESTEP) > HALF_BOUNDARY_WIDTH)
		agent->dz = -agent->dz*RESTITUTION_COEFFICIENT;

	// Integrate velocities
	agent->x = agent->x + agent->dx * TIMESTEP;
	agent->y = agent->y + agent->dy * TIMESTEP;
	agent->z = agent->z + agent->dz * TIMESTEP;
    
    return 0;
}

  


#endif //_FLAMEGPU_FUNCTIONS
