#include "Billboard.h"
#include "Graphics.h"
#include "GpuResourceUtils.h"
#include "Scene.h"

Billboard::Billboard(ID3D11Device* device, ID3D11ShaderResourceView* shader_resource_view)
{
    HRESULT hr = S_OK;

    // 頂点バッファ（6頂点分）
    {
        D3D11_BUFFER_DESC buffer_desc = {};
        buffer_desc.ByteWidth = sizeof(Vertex) * 6;
        buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = device->CreateBuffer(&buffer_desc, nullptr, vertexBuffer.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }

    // 入力レイアウト + VS
    {
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = GpuResourceUtils::LoadVertexShader(
            device, "Data/Shader/BillboardVS.cso",
            inputElementDesc, ARRAYSIZE(inputElementDesc),
            inputLayout.GetAddressOf(), vertexShader.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }

    // PS
    {
        hr = GpuResourceUtils::LoadPixelShader(
            device, "Data/Shader/BillboardPS.cso", pixelShader.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }

    // Constant Buffer
    GpuResourceUtils::CreateConstantBuffer<CameraCB>(device, &cameraConstantBuffer);

    // テクスチャ情報
    if (shader_resource_view)
    {
        shader_resource_view->AddRef();
        shaderResourceView = shader_resource_view;

        Microsoft::WRL::ComPtr<ID3D11Resource> resource;
        shaderResourceView->GetResource(resource.GetAddressOf());

        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
        hr = resource->QueryInterface<ID3D11Texture2D>(texture2d.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));

        D3D11_TEXTURE2D_DESC desc;
        texture2d->GetDesc(&desc);
        textureWidth = static_cast<float>(desc.Width);
        textureHeight = static_cast<float>(desc.Height);
    }
}

Billboard::Billboard(ID3D11Device* device, std::string filename)
{
    HRESULT hr = S_OK;

    // 頂点バッファ（6頂点分）
    {
        D3D11_BUFFER_DESC buffer_desc = {};
        buffer_desc.ByteWidth = sizeof(Vertex) * 6;
        buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
        buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = device->CreateBuffer(&buffer_desc, nullptr, vertexBuffer.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }

    // 入力レイアウト + VS
    {
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        hr = GpuResourceUtils::LoadVertexShader(
            device, "Data/Shader/BillboardVS.cso",
            inputElementDesc, ARRAYSIZE(inputElementDesc),
            inputLayout.GetAddressOf(), vertexShader.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }

    // PS
    {
        hr = GpuResourceUtils::LoadPixelShader(
            device, "Data/Shader/BillboardPS.cso", pixelShader.GetAddressOf());
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
    }

    // Constant Buffer
    GpuResourceUtils::CreateConstantBuffer<CameraCB>(device, &cameraConstantBuffer);

    // テクスチャの生成
    if (!filename.empty())
    {
        // テクスチャファイル読み込み
        D3D11_TEXTURE2D_DESC desc;
        hr = GpuResourceUtils::LoadTexture(device, filename, shaderResourceView.GetAddressOf(), &desc);
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
        textureWidth = static_cast<float>(desc.Width);
        textureHeight = static_cast<float>(desc.Height);
    }
    else
    {
        // ダミーテクスチャ生成
        D3D11_TEXTURE2D_DESC desc;
        hr = GpuResourceUtils::CreateDummyTexture(device, 0xFFFFFFFF, shaderResourceView.GetAddressOf(),
            &desc);
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
        textureWidth = static_cast<float>(desc.Width);
        textureHeight = static_cast<float>(desc.Height);
    }
}

void Billboard::Render(Scene* scene, const Vector3& position, float size, const Color& color, const ImFontGlyph* glyph)
{
    auto dc = Graphics::Instance().GetDeviceContext();
    auto cam = scene->GetMainCamera();

    Vector3 right = cam->GetRight() * (size * 0.5f);
    Vector3 up = cam->GetUp() * (size * 0.5f);

    Vector3 p0 = position - right + up;
    Vector3 p1 = position + right + up;
    Vector3 p2 = position - right - up;
    Vector3 p3 = position + right - up;

    // UVを指定範囲に変更
    Vertex vertices[6] =
    {
        { p0, color, {glyph->U0, glyph->V0} },  // 左上
        { p1, color, {glyph->U1, glyph->V0} },  // 右上
        { p2, color, {glyph->U0, glyph->V1} },  // 左下

        { p2, color, {glyph->U0, glyph->V1} },
        { p1, color, {glyph->U1, glyph->V0} },
        { p3, color, {glyph->U1, glyph->V1} }   // 右下
    };

    // 頂点バッファ更新
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
        memcpy(mapped.pData, vertices, sizeof(vertices));
        dc->Unmap(vertexBuffer.Get(), 0);
    }

    // シェーダー設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    dc->IASetInputLayout(inputLayout.Get());
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    dc->VSSetShader(vertexShader.Get(), nullptr, 0);
    dc->PSSetShader(pixelShader.Get(), nullptr, 0);
    dc->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf());

    // Camera Constant Buffer
    CameraCB cb;
    cb.view = cam->GetView();
    cb.proj = cam->GetProjection();

	auto rs = scene->GetRenderState();
    ID3D11SamplerState* samplers[]{
        rs->GetSamplerState(SamplerState::LinearWrap)
    };
	dc->PSSetSamplers(0, 1, samplers);

    dc->VSSetConstantBuffers(0, 1, cameraConstantBuffer.GetAddressOf());
    dc->PSSetConstantBuffers(0, 1, cameraConstantBuffer.GetAddressOf());
    dc->UpdateSubresource(cameraConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // レンダーステート（ビルボードはこれがおすすめ）
    dc->OMSetBlendState(rs->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(rs->GetDepthStencilState(DepthState::TestOnly), 0);  // Zテストのみ、書き込みなし
    dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullNone));

    dc->Draw(6, 0);
}

void Billboard::Render(Scene* scene, const Vector3& position, float size, const Color& color)
{
    auto dc = Graphics::Instance().GetDeviceContext();
    auto cam = scene->GetMainCamera();

    Vector3 right = cam->GetRight() * (size * 0.5f);
    Vector3 up = cam->GetUp() * (size * 0.5f);

    Vector3 p0 = position - right + up;
    Vector3 p1 = position + right + up;
    Vector3 p2 = position - right - up;
    Vector3 p3 = position + right - up;

    // UVを指定範囲に変更
    Vertex vertices[6] =
    {
        { p0, color, {0, 0} },  // 左上
        { p1, color, {1, 0} },  // 右上
        { p2, color, {0, 1} },  // 左下

        { p2, color, {0, 1} },
        { p1, color, {1, 0} },
        { p3, color, {1, 1} }   // 右下
    };

    // 頂点バッファ更新
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        HRESULT hr = dc->Map(vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        _ASSERT_EXPR(SUCCEEDED(hr), HRTrace(hr));
        memcpy(mapped.pData, vertices, sizeof(vertices));
        dc->Unmap(vertexBuffer.Get(), 0);
    }

    // シェーダー設定
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    dc->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &stride, &offset);
    dc->IASetInputLayout(inputLayout.Get());
    dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    dc->VSSetShader(vertexShader.Get(), nullptr, 0);
    dc->PSSetShader(pixelShader.Get(), nullptr, 0);
    dc->PSSetShaderResources(0, 1, shaderResourceView.GetAddressOf());

    // Camera Constant Buffer
    CameraCB cb;
    cb.view = cam->GetView();
    cb.proj = cam->GetProjection();

	auto rs = scene->GetRenderState();
    ID3D11SamplerState* samplers[]{
        rs->GetSamplerState(SamplerState::PointClamp)
    };
    dc->PSSetSamplers(0, 1, samplers);

    dc->VSSetConstantBuffers(0, 1, cameraConstantBuffer.GetAddressOf());
    dc->PSSetConstantBuffers(0, 1, cameraConstantBuffer.GetAddressOf());
    dc->UpdateSubresource(cameraConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

    // レンダーステート（ビルボードはこれがおすすめ）
    dc->OMSetBlendState(rs->GetBlendState(BlendState::Transparency), nullptr, 0xFFFFFFFF);
    dc->OMSetDepthStencilState(rs->GetDepthStencilState(DepthState::TestOnly), 0);  // Zテストのみ、書き込みなし
    dc->RSSetState(rs->GetRasterizerState(RasterizerState::SolidCullNone));

    dc->Draw(6, 0);
}
