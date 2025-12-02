#pragma once

#include <string>
#include <vector>
#include <DirectXMath.h>
#include <wrl.h>
#include <d3d11.h>

class Model
{
public:
	Model(ID3D11Device* device, const char* filename, float scaling = 1.0f);

	struct Material
	{
		std::string name;
		std::string diffuseTextureFileName;
		std::string normalTextureFileName;

		DirectX::XMFLOAT4 color = { 1, 1, 1, 1 };
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> diffuseMap;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalMap;
	};

	struct Vertex
	{
		DirectX::XMFLOAT3 position = { 0, 0, 0 };
		DirectX::XMFLOAT4 boneWeight = { 1, 0, 0, 0 };
		DirectX::XMUINT4 boneIndex = { 0, 0, 0, 0 };
		DirectX::XMFLOAT2 texcoord = { 0, 0 };
		DirectX::XMFLOAT3 normal = { 0, 0, 0 };
		DirectX::XMFLOAT3 tangent = { 0, 0, 0 };
	};

	struct Node
	{
		std::string name;
		int parentIndex;
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT4 rotation;
		DirectX::XMFLOAT3 scale;

		DirectX::XMFLOAT4X4 localTransform;
		DirectX::XMFLOAT4X4 globalTransform;
		DirectX::XMFLOAT4X4 worldTransform;
		Node* parent = nullptr;
		std::vector<Node*> children;
	};

	struct Bone
	{
		int nodeIndex;
		DirectX::XMFLOAT4X4 offsetTransform;
		Node* node = nullptr;
	};

	struct Mesh
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
		int materialIndex = 0;
		Material* material = nullptr;
		int nodeIndex = 0;
		Node* node = nullptr;
		std::vector<Bone> bones;
	};

	struct VectorKeyframe
	{
		float seconds;
		DirectX::XMFLOAT3 value;
	};

	struct QuaternionKeyframe
	{
		float seconds;
		DirectX::XMFLOAT4 value;
	};

	struct NodeAnim
	{
		std::vector<VectorKeyframe> positionKeyframes;
		std::vector<QuaternionKeyframe> rotationKeyframes;
		std::vector<VectorKeyframe> scaleKeyframes;
	};

	struct Animation
	{
		std::string name;
		float secondsLength;
		std::vector<NodeAnim> nodeAnims;
	};

	// ルートノード取得
	Node* GetRootNode() { return nodes.data(); }

	// メッシュデータ取得
	const std::vector<Mesh>& GetMeshes() const { return meshes; }

	// マテリアルデータ取得
	const std::vector<Material>& GetMaterials() const { return materials; }

	// トランスフォーム更新処理
	void UpdateTransform(const DirectX::XMFLOAT4X4& worldTransform);

	// アニメーション再生
	void PlayAnimation(int index, bool loop, float blendSeconds = 0);

	// アニメーション再生中か
	bool IsPlayAnimation() const;

	// アニメーション更新処理
	void UpdateAnimation(float elapsedTime);

	// アニメーションデータ取得
	const std::vector<Animation>& GetAnimations() const { return animations; }

private:

	// アニメーション計算処理
	void ComputeAnimation(float elapsedTime);

	// ブレンディング計算処理
	void ComputeBlending(float elapsedTime);

	std::vector<Mesh> meshes;
	std::vector<Material> materials;
	std::vector<Node> nodes;

	std::vector<Animation> animations;
	int currentAnimationIndex = -1;
	float currentAnimationSeconds = 0;
	bool animationPlaying = false;
	bool animationLoop = false;

	struct NodeCache
	{
		DirectX::XMFLOAT3 position = { 0, 0, 0 };
		DirectX::XMFLOAT4 rotation = { 0, 0, 0, 1 };
		DirectX::XMFLOAT3 scale = { 1, 1, 1 };
	};
	std::vector<NodeCache> nodeCaches;
	float currentAnimationBlendSeconds = 0.0f;
	float animationBlendSecondsLength = -1.0f;
	bool animationBlending = false;

	float scaling = 1.0f;
};