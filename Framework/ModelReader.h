#pragma once
#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include "MeshData.h"
#include "Renderer.h"

class MeshComponent;

class ModelReader
{
public:
	ModelReader(Renderer& renderer, const std::string& path);
	std::vector<MeshData> GetMeshes() const;

private:
	void ProcessNode(Renderer& renderer, aiNode* node, const aiScene* scene);
	MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);

private:
	std::vector<MeshData> meshData;
};

