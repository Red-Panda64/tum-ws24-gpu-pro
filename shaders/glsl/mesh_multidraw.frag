#version 460
#extension GL_EXT_nonuniform_qualifier: enable

struct DirLight
{
    vec3 direction;
    vec3 color;
};

struct PointLight
{
    vec3 position;
    vec3 color;
    vec3 attenuationFactors; // constant, linear and quadratic in order
};

#define MAX_NR_OF_POINT_LIGHTS 16
layout(set = 0, binding = 0) uniform Scene
{
    mat4 projectionView;
    vec3 camPos;
    DirLight dirLight;
    PointLight pointLights[MAX_NR_OF_POINT_LIGHTS];
    int nrPointLights;
    float ambientFactor;
} scene;

// Each mesh has 2 textures (diffuse and specular). Each mesh, can fetch its textures with index: 2 * gl_drawID
layout(set = 1, binding = 0) uniform sampler2D textures[];

layout(location = 0) in VIn
{
    vec3 normal;
    vec3 fragWorldPos; // for light calculations
	vec2 uv;
    flat int drawID;
} vIn;


layout(location = 0) out vec4 color;

int diffuseTextureIndex = 2 * vIn.drawID;
int specularTextureIndex = 2 * vIn.drawID + 1;


vec3 calculateDirLight(DirLight light, vec3 normal, vec3 fragToCamera, vec3 diffuseTextureValue, vec3 specularTextureValue)
{
	vec3 fragToLight = normalize(-light.direction);
	//Diffuse shading
	float diffCoeff = max(dot(fragToLight, normal), 0.0f);
	//Specular shading
	vec3 reflected = reflect(-fragToLight, normal);
	vec3 halfwayDir = normalize(fragToLight + fragToCamera);
	float specularCoeff = pow(max(dot(halfwayDir, normal), 0.0f), 32);
	//Combine results
	vec3 ambient = light.color * scene.ambientFactor * diffuseTextureValue;
	vec3 diffuse = light.color * diffCoeff * diffuseTextureValue;
	vec3 specular = light.color * specularCoeff * specularTextureValue;

	return (ambient + diffuse + specular);
}

vec3 calculatePointLight(PointLight light, vec3 normal, vec3 fragPosition, vec3 fragToCamera, vec3 diffuseTextureValue, vec3 specularTextureValue)
{
	vec3 fragToLight = normalize(light.position - fragPosition);
	//Diffuse shading
	float diffCoeff = max(dot(fragToLight, normal), 0.0f);
	//Specular shading
	vec3 reflected = reflect(-fragToLight, normal);
	vec3 halfwayDir = normalize(fragToLight + fragToCamera);
	float specularCoeff = pow(max(dot(halfwayDir, normal), 0.0f), 32);

	//Attenuation
	float dist = length(light.position - fragPosition);
	float attenuation = 1.0f / (light.attenuationFactors[0] + light.attenuationFactors[1] * dist + light.attenuationFactors[2] * dist * dist);

	//Combine
	vec3 ambient = light.color * scene.ambientFactor * diffuseTextureValue;
	vec3 diffuse = light.color * diffCoeff * diffuseTextureValue;
	vec3 specular = light.color * specularCoeff * specularTextureValue;

	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;
	
	return (ambient + diffuse + specular);

}

void main()
{
	//Fundamental variables
	vec3 n = normalize(vIn.normal);
	vec3 fragToCamera = normalize(scene.camPos - vIn.fragWorldPos);

	vec3 diffuseTextureValue = vec3(texture(textures[diffuseTextureIndex], vIn.uv));
	vec3 specularTextureValue = vec3(texture(textures[specularTextureIndex], vIn.uv));

	vec3 result = calculateDirLight(scene.dirLight, n, fragToCamera, diffuseTextureValue, specularTextureValue);

	for(int i = 0; i < scene.nrPointLights; ++i)
	{
		result += calculatePointLight(scene.pointLights[i], n, vIn.fragWorldPos, fragToCamera, diffuseTextureValue, specularTextureValue);
	}

	color = vec4(result, 1.0f);
}
