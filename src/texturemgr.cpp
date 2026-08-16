#define STB_IMAGE_IMPLEMENTATION
#include "pch.h"
#include "texturemgr.h"
#include "stb_image.h"
#include "wrapper.hpp"
#include <cstdio>

// Diagnostic logging into cleo_redux.log - texture failures used to be silent
static void TexLog(const char* fmt, const char* a, long long n) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), fmt, a, n);
    wLog(buf);
}

// d3dx9.h
extern "C"
{
    HRESULT WINAPI
    D3DXCreateTextureFromFileA(
        LPDIRECT3DDEVICE9         pDevice,
        LPCSTR                    pSrcFile,
        LPDIRECT3DTEXTURE9*       ppTexture);

    HRESULT WINAPI
    D3DXCreateTextureFromFileW(
        LPDIRECT3DDEVICE9         pDevice,
        LPCWSTR                   pSrcFile,
        LPDIRECT3DTEXTURE9*       ppTexture);

#ifdef UNICODE
#define D3DXCreateTextureFromFile D3DXCreateTextureFromFileW
#else
#define D3DXCreateTextureFromFile D3DXCreateTextureFromFileA
#endif
}

static bool D3D11CreateTextureFromFile(ID3D11Device* pDevice, const char* filename,
                                       ID3D11ShaderResourceView** outSrc) {
    // Load from disk into a raw RGBA buffer
    int width = 0;
    int height = 0;
    unsigned char* data = stbi_load(filename, &width, &height, NULL, 4);
    if (data == NULL) {
        TexLog("ImGuiRedux: stbi_load FAILED for '%s' (%lld)", filename, 0);
        return false;
    }
    TexLog("ImGuiRedux: stbi_load ok '%s' w*h=%lld", filename,
           (long long)width * 10000 + height);

    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D *pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;

    HRESULT hrTex = pDevice->CreateTexture2D(&desc, &subResource, &pTexture);
    if (FAILED(hrTex) || !pTexture) {
        TexLog("ImGuiRedux: CreateTexture2D FAILED for '%s' hr=%lld", filename, (long long)hrTex);
        stbi_image_free(data);
        return false;
    }

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    HRESULT hrSrv = pDevice->CreateShaderResourceView(pTexture, &srvDesc, outSrc);
    pTexture->Release();
    stbi_image_free(data);
    if (FAILED(hrSrv) || !*outSrc) {
        TexLog("ImGuiRedux: CreateSRV FAILED for '%s' hr=%lld", filename, (long long)hrSrv);
        return false;
    }
    TexLog("ImGuiRedux: texture READY '%s' (%lld)", filename, 1);
    return true;
}

void TextureMgr::LoadTexture(TextureInfo &info) {
    if (!gD3DDevice) {
        TexLog("ImGuiRedux: LoadTexture '%s' skipped - no device yet (%lld)",
               info.path.c_str(), 0);
        return;
    }

    if (gRenderer == eRenderer::Dx9) {
        D3DXCreateTextureFromFile(reinterpret_cast<IDirect3DDevice9*>(gD3DDevice), info.path.c_str(),
                                  reinterpret_cast<IDirect3DTexture9**>(&info.pTexture));
    } else if (gRenderer == eRenderer::Dx11) {
        D3D11CreateTextureFromFile(reinterpret_cast<ID3D11Device*>(gD3DDevice), info.path.c_str(),
                                   reinterpret_cast<ID3D11ShaderResourceView**>(&info.pTexture));
    }
}

TextureInfo* TextureMgr::LoadTextureFromPath(const char *path) {
    TextureInfo info;
    info.path = std::string(path);
    info.pTexture = nullptr;
    // FIX ME
    // LoadTexture(info);
    textureList.push_back(std::move(info));
    return &textureList.back();
}

void TextureMgr::FreeTexture(TextureInfo *pInfo) {
    if (!pInfo) {
        return;
    }

    if (gRenderer == eRenderer::Dx9) {
        reinterpret_cast<IDirect3DTexture9*>(pInfo->pTexture)->Release();
    } else if (gRenderer == eRenderer::Dx11) {
        reinterpret_cast<ID3D11ShaderResourceView*>(pInfo->pTexture)->Release();
    }

    // copy first: pInfo points INTO the list, and list::remove erasing the
    // element its own argument references is unspecified behaviour
    TextureInfo target = *pInfo;
    textureList.remove(target);   // std::remove alone never erased anything
}

TextureInfo* TextureMgr::FindInfo(std::string &&path) {
    for (auto &item : textureList) {
        if (item.path == path) {
            return &item;
        }
    }

    return nullptr;
}

bool TextureMgr::Exists(TextureInfo *pInfo) {
    for (TextureInfo item : textureList) {
        if (item.path == pInfo->path) {
            return true;
        }
    }
    return false;
}
