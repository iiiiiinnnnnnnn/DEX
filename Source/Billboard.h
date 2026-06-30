#pragma once
#include <Common.h>

class Scene;

class Billboard
{
public:
    Billboard(ID3D11Device* device, ID3D11ShaderResourceView* shader_resource_view);
    Billboard(ID3D11Device* device, std::string filename);

    void Render(Scene* scene, const Vector3& position, float size, const Color& color, const ImFontGlyph* glyph);
    void Render(Scene* scene, const Vector3& position, float size, const Color& color);

    ID3D11ShaderResourceView* GetSRV() {
        return shaderResourceView.Get();
    }

private:
    // 頂点データ
    struct Vertex
    {
        Vector3 position;
        Color color;
        Vector2 texcoord;
    };

    struct CameraCB
    {
        Matrix view;
        Matrix proj;
    };
    Microsoft::WRL::ComPtr<ID3D11Buffer> cameraConstantBuffer;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shaderResourceView;

    float textureWidth = 0;
    float textureHeight = 0;
};