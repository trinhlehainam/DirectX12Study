#include "Material.h"

Material::Material(DirectX::XMFLOAT4 DiffuseAlbedo_, DirectX::XMFLOAT3 FresnelF0_, float Roughness_):
	DiffuseAlbedo(DiffuseAlbedo_), FresnelF0(FresnelF0_), Roughness(Roughness_)
{
}
