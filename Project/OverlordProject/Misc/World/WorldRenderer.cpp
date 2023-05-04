#include "stdafx.h"
#include "WorldRenderer.h"

void WorldRenderer::SetBuffers(std::vector<Chunk>& chunks, const SceneContext& sceneContext, bool instantApply)
{
	for (Chunk& chunk : chunks)
	{
		if(chunk.verticesChanged) SetBuffer(chunk, sceneContext, instantApply);
	}
}

void WorldRenderer::SetBuffer(Chunk& chunk, const SceneContext& sceneContext, bool instantApply)
{
	chunk.verticesChanged = false;

	auto& vertices{ chunk.vertices };

	if (vertices.size() == 0) return;

	std::stable_partition(begin(vertices), end(vertices), [](const VertexPosNormTexTransparency& v) { return !v.Transparent; });
	const int nrTransparent{ static_cast<int>(std::count_if(begin(vertices), end(vertices), [](const VertexPosNormTexTransparency& v) {return v.Transparent; })) };

	//*************
	//VERTEX BUFFER
	D3D11_BUFFER_DESC vertexBuffDesc{};
	vertexBuffDesc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER;
	vertexBuffDesc.ByteWidth = static_cast<UINT>(sizeof(VertexPosNormTexTransparency) * (vertices.size() - nrTransparent));
	vertexBuffDesc.CPUAccessFlags = D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE;
	vertexBuffDesc.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
	vertexBuffDesc.MiscFlags = 0;

	ID3D11Buffer** pVBuffer{ instantApply ? &chunk.pVertexBuffer : &chunk.pBackVertexBuffer };
	int& vBufferSize{ instantApply ? chunk.vertexBufferSize : chunk.backVertexBufferSize };
	ID3D11Buffer** pVTransparentBuffer{ instantApply ? &chunk.pVertexTransparentBuffer : &chunk.pBackVertexTransparentBuffer };
	int& vTransparentBufferSize{ instantApply ? chunk.vertexTransparentBufferSize : chunk.backVertexTransparentBufferSize };

	if (instantApply && *pVBuffer) SafeRelease((*pVBuffer));
	if (instantApply && *pVTransparentBuffer) SafeRelease((*pVTransparentBuffer));

	D3D11_SUBRESOURCE_DATA initData{};
	initData.pSysMem = vertices.data();

	vBufferSize = static_cast<int>(chunk.vertices.size()) - nrTransparent;
	if (vBufferSize > 0) sceneContext.d3dContext.pDevice->CreateBuffer(&vertexBuffDesc, &initData, pVBuffer);

	vertexBuffDesc.ByteWidth = static_cast<UINT>(sizeof(VertexPosNormTexTransparency) * nrTransparent);

	std::stable_partition(begin(vertices), end(vertices), [](const VertexPosNormTexTransparency& v) { return v.Transparent; });

	vTransparentBufferSize = nrTransparent;
	if(vTransparentBufferSize > 0) sceneContext.d3dContext.pDevice->CreateBuffer(&vertexBuffDesc, &initData, pVTransparentBuffer);
}

WorldRenderer::~WorldRenderer()
{
	SafeRelease(m_pInputLayout);
}

void WorldRenderer::LoadEffect(const SceneContext& sceneContext)
{
	m_pEffect = ContentManager::Load<ID3DX11Effect>(L"Effects\\World.fx");
	m_pDefaultTechnique = m_pEffect->GetTechniqueByIndex(0);
	m_pTransparentTechnique = m_pEffect->GetTechniqueByIndex(1);
	EffectHelper::BuildInputLayout(sceneContext.d3dContext.pDevice, m_pDefaultTechnique, &m_pInputLayout);

	m_pWorldVar = m_pEffect->GetVariableBySemantic("World")->AsMatrix();

	m_pWvpVar = m_pEffect->GetVariableBySemantic("WorldViewProjection")->AsMatrix();

	m_pLightWvpVar = m_pEffect->GetVariableByName("gLightWorldViewProj")->AsMatrix();

	m_pShadowMapVariable = m_pEffect->GetVariableByName("gShadowMap")->AsShaderResource();

	m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource()->SetResource(ContentManager::Load<TextureData>(L"Textures\\TileAtlas.dds")->GetShaderResourceView());

	m_pLightDirVar = m_pEffect->GetVariableByName("gLightDirection")->AsVector();
}

void WorldRenderer::Draw(std::vector<Chunk>& chunks, const SceneContext& sceneContext)
{
	const D3D11Context& deviceContext{ sceneContext.d3dContext };

	constexpr XMFLOAT4X4 worldMatrix{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
	const auto viewProjection = XMLoadFloat4x4(&sceneContext.pCamera->GetViewProjection());

	m_pWorldVar->SetMatrix(reinterpret_cast<float*>(&world));
	m_pWvpVar->SetMatrix(reinterpret_cast<const float*>(&viewProjection));
	m_pLightWvpVar->SetMatrix(reinterpret_cast<const float*>(&ShadowMapRenderer::Get()->GetLightVP()));

	// Update the ShadowMap texture
	m_pShadowMapVariable->SetResource(ShadowMapRenderer::Get()->GetShadowMap());

	// Update the Light Direction (retrieve the direction from the LightManager > sceneContext)
	m_pLightDirVar->SetFloatVector(reinterpret_cast<const float*>(&sceneContext.pLights->GetDirectionalLight().direction));

	deviceContext.pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext.pDeviceContext->IASetInputLayout(m_pInputLayout);

	constexpr UINT offset = 0;
	constexpr UINT stride = sizeof(VertexPosNormTexTransparency);

	for (const Chunk& chunk : chunks)
	{
		deviceContext.pDeviceContext->IASetVertexBuffers(0, 1, &chunk.pVertexBuffer, &stride, &offset);

		D3DX11_TECHNIQUE_DESC techDesc{};
		m_pDefaultTechnique->GetDesc(&techDesc);
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			m_pDefaultTechnique->GetPassByIndex(p)->Apply(0, deviceContext.pDeviceContext);
			deviceContext.pDeviceContext->Draw(static_cast<UINT>(chunk.vertexBufferSize), 0);
		}
	}

	for (const Chunk& chunk : chunks)
	{
		deviceContext.pDeviceContext->IASetVertexBuffers(0, 1, &chunk.pVertexTransparentBuffer, &stride, &offset);

		D3DX11_TECHNIQUE_DESC techDesc{};
		m_pTransparentTechnique->GetDesc(&techDesc);
		for (UINT p = 0; p < techDesc.Passes; ++p)
		{
			m_pTransparentTechnique->GetPassByIndex(p)->Apply(0, deviceContext.pDeviceContext);
			deviceContext.pDeviceContext->Draw(static_cast<UINT>(chunk.vertexTransparentBufferSize), 0);
		}
	}
}

void WorldRenderer::Draw(Chunk& chunk, const SceneContext& sceneContext)
{
	const D3D11Context& deviceContext{ sceneContext.d3dContext };

	constexpr XMFLOAT4X4 worldMatrix{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	XMMATRIX world = XMLoadFloat4x4(&worldMatrix);
	const auto viewProjection = XMLoadFloat4x4(&sceneContext.pCamera->GetViewProjection());

	m_pWorldVar->SetMatrix(reinterpret_cast<float*>(&world));
	m_pWvpVar->SetMatrix(reinterpret_cast<const float*>(&viewProjection));
	m_pLightWvpVar->SetMatrix(reinterpret_cast<const float*>(&ShadowMapRenderer::Get()->GetLightVP()));

	// Update the ShadowMap texture
	m_pShadowMapVariable->SetResource(ShadowMapRenderer::Get()->GetShadowMap());

	// Update the Light Direction (retrieve the direction from the LightManager > sceneContext)
	m_pLightDirVar->SetFloatVector(reinterpret_cast<const float*>(&sceneContext.pLights->GetDirectionalLight().direction));

	deviceContext.pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext.pDeviceContext->IASetInputLayout(m_pInputLayout);

	constexpr UINT offset = 0;
	constexpr UINT stride = sizeof(VertexPosNormTexTransparency);

	deviceContext.pDeviceContext->IASetVertexBuffers(0, 1, &chunk.pVertexBuffer, &stride, &offset);

	D3DX11_TECHNIQUE_DESC techDesc{};
	m_pDefaultTechnique->GetDesc(& techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		m_pDefaultTechnique->GetPassByIndex(p)->Apply(0, deviceContext.pDeviceContext);
		deviceContext.pDeviceContext->Draw(static_cast<UINT>(chunk.vertexBufferSize), 0);
	}

	deviceContext.pDeviceContext->IASetVertexBuffers(0, 1, &chunk.pVertexTransparentBuffer, &stride, &offset);

	m_pTransparentTechnique->GetDesc(&techDesc);
	for (UINT p = 0; p < techDesc.Passes; ++p)
	{
		m_pTransparentTechnique->GetPassByIndex(p)->Apply(0, deviceContext.pDeviceContext);
		deviceContext.pDeviceContext->Draw(static_cast<UINT>(chunk.vertexTransparentBufferSize), 0);
	}
}

void WorldRenderer::DrawShadowMap(const Chunk& chunk, const SceneContext& sceneContext)
{
	constexpr UINT stride = sizeof(VertexPosNormTexTransparency);
	constexpr XMFLOAT4X4 worldMatrix{ 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	ShadowMapRenderer::Get()->DrawMesh(sceneContext, chunk.pVertexBuffer, chunk.vertexBufferSize, stride, worldMatrix);
}
