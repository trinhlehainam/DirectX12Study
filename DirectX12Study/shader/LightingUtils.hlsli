
static const uint MAX_LIGHTS = 16;

struct Light
{
	float3 Strength;
	float FallOffStart;
	float3 Direction;
	float FallOffEnd;
	float3 Position;
	float SpotPower;
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

float3 SchlickFresnel(float3 FresnelF0, float3 lightDirection, float3 normal)
{
	float cosIncidentAngle = saturate(dot(-lightDirection, normal));
	
	// R0 + (1-R0) x (1-cosIncidentAngle)^5
	return FresnelF0 + (1 - FresnelF0) * pow((1 - cosIncidentAngle), 5);
}

float CalculateRoughness(float m, float3 lightDirection, float3 viewVector , float3 faceNormal)
{
	float3 halfVec = normalize(-lightDirection + (-viewVector));
	
	return (m + 8) * pow(max(dot(halfVec, faceNormal), 0), m) / m;
}

float3 BlinnPhong(float3 lightStrength, float3 lightDirection, float3 viewVector, 
	float3 normal, Material	material)
{
	float roughtness = CalculateRoughness(material.Shinness * 256.0f, lightDirection, viewVector, normal);
	float3 fresnel = SchlickFresnel(material.FresnelF0, lightDirection, normal);
	float3 specularAlbedo = fresnel * roughtness;
	
	// Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
	specularAlbedo = specularAlbedo / (specularAlbedo + 1.0f);
	
	return (material.DiffuseAlbedo.xyz + specularAlbedo) * lightStrength;
}

float3 ComputeDirectionalLight(Light light, Material material, float3 viewVector, float3 normal)
{
	float3 lightStrength = light.Strength * max(dot(-light.Direction, normal), 0);
	
	return BlinnPhong(lightStrength, light.Direction, viewVector, normal, material);
}