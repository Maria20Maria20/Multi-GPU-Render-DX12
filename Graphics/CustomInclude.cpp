#include "CustomInclude.h"
#include <filesystem>
#include <fstream>

CustomInclude::CustomInclude(std::wstring baseDir): m_baseDir(std::move(baseDir)) {}

HRESULT CustomInclude::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID* ppData,
                            UINT* pBytes)
{
    if (!pFileName || !ppData || !pBytes)
        return E_INVALIDARG;
    
    // 1) сначала пробуем относительный путь от папки сабмодуля
    std::wstring wname(pFileName, pFileName + strlen(pFileName));
    std::filesystem::path fullPath = std::filesystem::path(m_baseDir) / wname;

    std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
    if (file.is_open())
    {
        size_t size = (size_t)file.tellg();
        file.seekg(0, std::ios::beg);

        char* data = (char*)malloc(size ? size : 1);
        if (size) file.read(data, size);

        *ppData = data;
        *pBytes = (UINT)size;
        return S_OK;
    }

    // 2) если не нашли – НИЧЕГО не делаем, просто возвращаем ошибку
    *ppData = nullptr;
    *pBytes = 0;
    return D3D_COMPILE_STANDARD_FILE_INCLUDE->Open(
        IncludeType, pFileName, pParentData, ppData, pBytes);
}

HRESULT CustomInclude::Close(LPCVOID pData)
{
    if (pData)
        free((void*)pData);
    return S_OK;
}
