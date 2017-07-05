#ifndef PREAMBLE_GLSL
#define PREAMBLE_GLSL


// VERTEX ATTRIBUTES: layout(location = _____)

#define POSITION_ATTRIB_LOCATION				0
#define NORMAL_ATTRIB_LOCATION					1
#define TEXCOORD_ATTRIB_LOCATION				2
#define COLOR_ATTRIB_LOCATION					3
#define INSTANCE_POSITION_ATTRIB_LOCATION		4
#define INSTANCE_FORWARD_ATTRIB_LOCATION		5


// SHADER UNIFORMS: layout(location = _____)

#define MODEL_MAT_UNIFORM_LOCATION				0
#define MATERIAL_SHININESS_UNIFORM_LOCATION		1
#define LIGHT_COUNT_UNIFORM_LOCATION			2


// UNIFORM BLOCKS: layout(std40, binding = _____)

#define SCENE_UNIFORM_BUFFER_LOCATION			0
#define LIGHTS_UNIFORM_BUFFER_LOCATION			1


// TEXTURE UNITS: layout(binding = _____)

#define DIFFUSE_TEXTURE_BINDING					0
#define SPECULAR_TEXTURE_BINDING				1
#define EMISSIVE_TEXTURE_BINDING				2


// LIGHTING DEFINITIONS
#define MAX_LIGHTS 10

#endif // PREAMBLE_GLSL