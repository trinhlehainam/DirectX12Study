
static const uint MAX_LIGHTS = 16;

struct Light
{
	float3 Strength;
	float FallOffStart;
	float3 Direction;
	float FallOffEnd;
	float3 Position;
	float SpotPower;
	float4x4 ProjectMatrix;
};

struct Material
{
	float4 DiffuseAlbedo;
	float3 FresnelF0;
	float Shinness;
};
	
// Calculate the brighness percentage depend on distance of (spot /point light)
float CalculateAttenuation(float FallOfStart, float FallOfEnd, float Distance)
{
	// clamp value to (0 ~ 1)
	return saturate((Distance - FallOfStart) / (FallOfEnd - FallOfStart));
}

float3 SchlickFresnel(float3 FresnelF0, float3 lightUnitVector, float3 normalUnitVector)
{
	float cosIncidentAngle = saturate(dot(-lightUnitVector, normalUnitVector));
	
	// R0 + (1-R0) x (1-cosIncidentAngle)^5
	return FresnelF0 + (1.0f - FresnelF0) * pow((1 - cosIncidentAngle), 5.0f);
}

float CalculateRoughness(float m, float3 lightUnitVector, float3 viewUnitVector, float3 normalUnitVector)
{
	float3 halfVec = normalize(-lightUnitVector - viewUnitVector);
	
	return (m + 8.0f) * pow(max(dot(halfVec, normalUnitVector), 0.0f), m) / 8.0f;
}

float3 BlinnPhong(float3 lightStrength, float3 lightUnitVector, float3 viewVector, 
	float3 normal, Material	material)
{
	float roughtness = CalculateRoughness(material.Shinness * 256.0f, lightUnitVector, viewVector, normal);
	float3 fresnel = SchlickFresnel(material.FresnelF0, lightUnitVector, normal);
	float3 specularAlbedo = fresnel * roughtness;
	
	// Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
	specularAlbedo = specularAlbedo / (specularAlbedo + 1.0f);
	
	return (material.DiffuseAlbedo.xyz + specularAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light light, Material material, float3 viewUnitVector, float3 normalUnitVector)
{
	float3 lightUnitVec = normalize(light.Direction);
	
	float3 lightStrength = light.Strength * max(dot(-lightUnitVec, normalUnitVector), 0.0f);
	
	return BlinnPhong(lightStrength, lightUnitVec, viewUnitVector, normalUnitVector, material);
}

float4 ComputeLighting(Light lights[MAX_LIGHTS], Material material, float3 viewVector, float3 normalVector)
{
	float4 ret = float4(0.0f, 0.0f, 0.0f, 1.0f);
	
	uint i = 0;
	
	float3 normalUnitVec = normalize(normalVector);
	float3 viewUnitVec = normalize(viewVector);
	
#ifdef NUM_DIRECTIONAL_LIGHT
	for (i = 0; i < NUM_DIRECTIONAL_LIGHT; ++i)
	{
		ret.rgb += ComputeDirectionalLight(lights[i], material, viewUnitVec, normalUnitVec);
	};
#endif
	
	return ret;

}