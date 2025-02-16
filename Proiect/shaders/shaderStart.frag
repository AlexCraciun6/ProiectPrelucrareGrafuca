#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;

out vec4 fColor;
in vec4 fragPosLightSpace;

//lighting
uniform	vec3 lightDir;
uniform	vec3 lightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

//fog
uniform float fogDensityFactor;

//showLight
uniform bool showLight;

in vec3 fragPos;
// Add these uniforms at the top of your fragment shader
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform float spotLightCutoff;
uniform float spotLightOuterCutoff;

uniform mat4 view;

void computeLightComponents()
{		
    vec3 cameraPosEye = vec3(0.0f);
    vec3 normalEye = normalize(fNormal);	
    vec3 lightDirN = normalize(lightDir);
    vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);

    // Base light calculation
    vec3 effectiveLightColor = showLight ? lightColor : vec3(0.0);
    
    // Directional light components
    ambient = ambientStrength * effectiveLightColor;
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * effectiveLightColor;
    vec3 reflection = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
    specular = specularStrength * specCoeff * effectiveLightColor;

    // Spotlight calculation
    if (showLight) {
        // Transform spotlight vectors to view space
        vec3 spotLightPosView = vec3(view * vec4(spotLightPos, 1.0));
        vec3 spotLightDirView = normalize(vec3(view * vec4(spotLightDir, 0.0)));
        
        // Calculate light direction from fragment to spotlight
        vec3 spotLightToFrag = normalize(spotLightPosView - fPosEye.xyz);
        
        // Calculate spot light angle
        float theta = dot(spotLightToFrag, normalize(-spotLightDirView));
        float epsilon = spotLightCutoff - spotLightOuterCutoff;
        float intensity = clamp((theta - spotLightOuterCutoff) / epsilon, 0.0, 1.0);

        // Add spotlight contribution if fragment is within the light cone
        if(theta > spotLightOuterCutoff) 
        {
            vec3 spotReflection = reflect(-spotLightToFrag, normalEye);
            float spotSpecCoeff = pow(max(dot(viewDirN, spotReflection), 0.0f), shininess);
            
            // Increase spotlight intensity
            vec3 spotColor = lightColor * 2.0; // Make spotlight brighter
            
            // Add spotlight contribution to existing lighting
            diffuse += intensity * max(dot(normalEye, spotLightToFrag), 0.0f) * spotColor;
            specular += intensity * spotSpecCoeff * specularStrength * spotColor;
        }
    }
}

float computeShadow()
{

	vec3 normalizedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	normalizedCoords = normalizedCoords * 0.5 + 0.5;

	if (normalizedCoords.z > 1.0f)
		return 0.0f;

	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	float currentDepth = normalizedCoords.z;
	
	float bias = 0.005f;
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

	return shadow;
}

float fog() {
    //float fogDensity = 0.01f; // for testing
    float fragmentDistance = length(fPosEye);
    float fogFactor = exp(-fragmentDistance * fogDensityFactor);
    return clamp(fogFactor, 0.0f, 1.0f);
}

void main() 
{
	computeLightComponents();
	
	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;

	float shadow = computeShadow();
    //vec3 color = min((ambient + (1.0f - shadow)*diffuse) + (1.0f - shadow)*specular, 1.0f);
    float alpha = texture(diffuseTexture, fTexCoords).a;
	vec4 color = vec4( min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow)*specular, 1.0f), alpha);

	vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
	float fogFactor = fog();
	fColor = mix(color, fogColor, 1.0 - fogFactor);
}
