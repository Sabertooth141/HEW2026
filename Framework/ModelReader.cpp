#include "ModelReader.h"

#include <assimp/postprocess.h>

ModelReader::ModelReader(Renderer& renderer, const std::string& path)
{
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(
		path,
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_ConvertToLeftHanded |
		aiProcess_GenNormals
	);

	if (!scene || !scene->mRootNode)
	{
		std::string err = importer.GetErrorString();
		OutputDebugStringA(err.c_str());

		assert(false);
	}

	ProcessNode(renderer, scene->mRootNode, scene);
}

std::vector<MeshData> ModelReader::GetMeshes() const
{
	return meshData;
}

void ModelReader::ProcessNode(Renderer& renderer, aiNode* node, const aiScene* scene)
{
	for (unsigned i = 0; i < node->mNumMeshes; i++)
	{
		auto mesh = scene->mMeshes[node->mMeshes[i]];
		meshData.push_back(ProcessMesh(mesh, scene));
	}

	for (unsigned i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(renderer, node->mChildren[i], scene);
	}
}

MeshData ModelReader::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	MeshData result;

	// vertices
	for (unsigned i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex v;

		v.pos = {
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		};

		v.normal = {
			mesh->mNormals[i].x,
			mesh->mNormals[i].y,
			mesh->mNormals[i].z
		};

		if (mesh->mTextureCoords[0])
		{
			v.texcoord = {
				mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y
			};
		}
		else
		{
			v.texcoord = {0, 0};
		}

		result.vertices.push_back(v);
	}
	// texture
	if (mesh->mMaterialIndex < scene->mNumMaterials)
	{
		
	}

	// indices
	for (unsigned i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (unsigned j = 0; j < face.mNumIndices; j++)
		{
			result.indices.push_back(face.mIndices[j]);
		}
	}

	return result;
}
