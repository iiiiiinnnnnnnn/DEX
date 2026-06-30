#pragma once

#include "Component.h"
#include "Material.h"
#include "Transform.h"
#include "Light.h"

namespace DirectX
{
	template<class Archive>
	void serialize(Archive& archive, XMUINT4& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z),
			cereal::make_nvp("w", v.w)
		);
	}
	template<class Archive>
	void serialize(Archive& archive, XMFLOAT2& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y)
		);
	}
	template<class Archive>
	void serialize(Archive& archive, XMFLOAT3& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z)
		);
	}
	template<class Archive>
	void serialize(Archive& archive, XMFLOAT4& v)
	{
		archive(
			cereal::make_nvp("x", v.x),
			cereal::make_nvp("y", v.y),
			cereal::make_nvp("z", v.z),
			cereal::make_nvp("w", v.w)
		);
	}
	template<class Archive>
	void serialize(Archive& archive, XMFLOAT4X4& m)
	{
		archive(
			cereal::make_nvp("_11", m._11),
			cereal::make_nvp("_12", m._12),
			cereal::make_nvp("_13", m._13),
			cereal::make_nvp("_14", m._14),
			cereal::make_nvp("_21", m._21),
			cereal::make_nvp("_22", m._22),
			cereal::make_nvp("_23", m._23),
			cereal::make_nvp("_24", m._24),
			cereal::make_nvp("_31", m._31),
			cereal::make_nvp("_32", m._32),
			cereal::make_nvp("_33", m._33),
			cereal::make_nvp("_34", m._34),
			cereal::make_nvp("_41", m._41),
			cereal::make_nvp("_42", m._42),
			cereal::make_nvp("_43", m._43),
			cereal::make_nvp("_44", m._44)
		);
	}
}

class Model : public Component{
private:
	friend class GameObject;
	void OnInspector() override;
public:
	Model() = default;
	Model(const std::string& mdlpath);

	DEFINE_COMPONENT_TYPE(ICON_FA_SHAPES, モデル)

	struct Node
	{
		std::string name;
		int parentIndex;

		Vector3 position;
		Quaternion rotation;
		Vector3 scale;

		Matrix localTransform;
		Matrix globalTransform;
		Matrix worldTransform;

		Node* parent = nullptr;
		std::vector<Node*> children;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(
				CEREAL_NVP(name),
				CEREAL_NVP(parentIndex),
				CEREAL_NVP(position),
				CEREAL_NVP(rotation),
				CEREAL_NVP(scale)
			);
		}
	};

	static const int MAX_BONE_INFLUENCES{ 4 };
	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal{ 0, 1, 0 };
		DirectX::XMFLOAT4 tangent{ 1, 0, 0, 1 };
		DirectX::XMFLOAT2 texcoord{ 0, 0 };
		float bone_weights[MAX_BONE_INFLUENCES]{ 1, 0, 0, 0 };
		uint32_t bone_indices[MAX_BONE_INFLUENCES]{};

		template <class T>
		void serialize(T& archive)
		{
			archive(
				CEREAL_NVP(position),
				CEREAL_NVP(normal),
				CEREAL_NVP(tangent),
				CEREAL_NVP(texcoord),
				CEREAL_NVP(bone_weights),
				CEREAL_NVP(bone_indices)
			);
		}
	};

	struct Bone
	{
		int nodeIndex;
		Matrix offsetTransform;
		Node* node = nullptr;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(
				CEREAL_NVP(nodeIndex),
				CEREAL_NVP(offsetTransform)
			);
		}
	};

	struct Mesh
	{
		std::string name;
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
		int materialIndex = 0;
		int nodeIndex = 0;
		Node* node = nullptr;
		std::vector<Bone> bones;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(
				CEREAL_NVP(name),
				CEREAL_NVP(vertices),
				CEREAL_NVP(indices),
				CEREAL_NVP(bones),
				CEREAL_NVP(nodeIndex),
				CEREAL_NVP(materialIndex)
			);
		}
	};

	struct VectorKeyframe
	{
		float seconds;
		Vector3 value;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(
				CEREAL_NVP(seconds),
				CEREAL_NVP(value)
			);
		}
	};

	struct QuaternionKeyframe
	{
		float seconds;
		Quaternion value;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(
				CEREAL_NVP(seconds),
				CEREAL_NVP(value)
			);
		}
	};

	struct NodeAnim
	{
		std::vector<VectorKeyframe> positionKeyframes;
		std::vector<QuaternionKeyframe> rotationKeyframes;
		std::vector<VectorKeyframe> scaleKeyframes;

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(
				CEREAL_NVP(positionKeyframes),
				CEREAL_NVP(rotationKeyframes),
				CEREAL_NVP(scaleKeyframes)
			);
		}
	};

	struct Animation
	{
		std::string name;
		float secondsLength;
		std::vector<NodeAnim> nodeAnims;

		static Animation LoadFromFilePath(std::string path)
		{
			Model::Animation animation;
			std::ifstream istream(path, std::ios::binary);
			if (istream.is_open())
			{
				cereal::BinaryInputArchive archive(istream);
				try {
					archive(
						CEREAL_NVP(animation)
					);
				}
				catch (...)
				{
					//_ASSERT_EXPR_A(false, "Anim deserialize failed.");
					animation = Animation();
				}
			}
			else
			{
				//_ASSERT_EXPR_A(false, "Anim File not found.");
				animation = Animation();
			}
			return animation;
		}

		template<class Archive>
		void serialize(Archive& archive)
		{
			archive(
				CEREAL_NVP(name),
				CEREAL_NVP(secondsLength),
				CEREAL_NVP(nodeAnims)
			);
		}
	};

	// ルートノード取得
	Node* GetRootNode() { return nodes.data(); }

	// メッシュデータ取得
	const std::vector<Mesh>& GetMeshes() const { return meshes; }

	// トランスフォーム更新処理
	void UpdateTransform(const Transform& worldTransform);

	void AppendAnimation(std::string path);

	// アニメーション再生
	void PlayAnimation(int index, bool loop, float blendSeconds = 0);

	// アニメーション再生中か
	bool IsPlayAnimation() const;

	// アニメーション更新処理
	void UpdateAnimation(float elapsedTime);

	// アニメーションデータ取得
	const std::vector<Animation>& GetAnimations() const { return animations; }

	// アニメーション計算処理
	void ComputeAnimation(float elapsedTime);

	// ブレンディング計算処理
	void ComputeBlending(float elapsedTime);

	std::vector<Node> nodes;
	std::vector<Mesh> meshes;
	std::vector<std::string> materialSlotNames;
	std::vector<std::string> materialPathes;
	std::vector<Animation> animations;
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> textureSRVs;

	std::string mdlpath;

	bool dirty = false;

	int currentAnimationIndex = -1;
	float currentAnimationSeconds = 0;
	bool animationPlaying = false;
	bool animationLoop = false;

	struct NodeCache
	{
		Vector3 position = { 0, 0, 0 };
		Quaternion rotation = { 0, 0, 0, 1 };
		Vector3 scale = { 1, 1, 1 };
	};
	std::vector<NodeCache> nodeCaches;
	float currentAnimationBlendSeconds = 0.0f;
	float animationBlendSecondsLength = -1.0f;
	bool animationBlending = false;

	void CtorMdl();

	void Update(float elapsedTime) override;
	void Render(Scene* scene) override;

	void Serialize(json& j) override {
		Component::Serialize(j);
		j["mdlpath"] = mdlpath;
	}

	void Deserialize(const json& j) override {
		Component::Deserialize(j);
		mdlpath = j["mdlpath"];
		CtorMdl();
	}

	static const uint32_t MAX_BONES = 256;
	struct SKINNING_CONSTANT_BUFFER // b0
	{
		Matrix boneTransforms[MAX_BONES];
	};

	struct OBJECT_CONSTANT_BUFFER // b1
	{
		Matrix world;
	};

	struct SCENE_CONSTANT_BUFFER // b2
	{
		Matrix view_projection;
		Matrix light_view_projection;
		Vector3 camera_position;
		float shadowTexelSize;
	};

	struct LIGHT_CONSTANT_BUFFER // b3
	{
		DIRECTIONAL_LIGHT_STRUCT directionLight;
		POINT_LIGHT_STRUCT pointLight[10];
		SPOT_LIGHT_STRUCT spotLight[10];
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer> skinningConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> objectConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> lightConstantBuffer;

	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
};