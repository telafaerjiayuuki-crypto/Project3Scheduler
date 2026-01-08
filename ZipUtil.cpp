#include "ZipUtil.h"  // 改为相对路径
#include <Windows.h>
#include <ShlDisp.h>
#include <comdef.h>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

#pragma comment(lib, "OleAut32.lib")
#pragma comment(lib, "Shell32.lib")

class ComInitRAII {
public:
    ComInitRAII() : hr_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
    ~ComInitRAII() { if (SUCCEEDED(hr_)) CoUninitialize(); }
    HRESULT Hr() const { return hr_; }
private:
    HRESULT hr_;
};

// 创建空 zip 文件
static bool CreateEmptyZip(const std::filesystem::path& zipPath) {
    static const unsigned char zipHeader[] = {
        0x50,0x4B,0x05,0x06, 0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00, 0x00,0x00
    };
    std::ofstream ofs(zipPath, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs.write(reinterpret_cast<const char*>(zipHeader), sizeof(zipHeader));
    return ofs.good();
}

static void SafeRelease(IUnknown* p) {
    if (p) p->Release();
}

bool ZipDirectoryShell(const std::filesystem::path& sourceDir,
    const std::filesystem::path& zipPath,
    std::string* errMsg) {
    try {
        if (!std::filesystem::exists(sourceDir) || !std::filesystem::is_directory(sourceDir)) {
            if (errMsg) *errMsg = "Source directory not found.";
            return false;
        }

        std::filesystem::create_directories(zipPath.parent_path());
        if (std::filesystem::exists(zipPath)) std::filesystem::remove(zipPath);

        if (!CreateEmptyZip(zipPath)) {
            if (errMsg) *errMsg = "Failed to create empty zip file.";
            return false;
        }

        ComInitRAII com;
        if (FAILED(com.Hr())) {
            if (errMsg) *errMsg = "CoInitializeEx failed.";
            return false;
        }

        IShellDispatch* shell = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_Shell, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&shell));
        if (FAILED(hr) || !shell) {
            if (errMsg) *errMsg = "CoCreateInstance(CLSID_Shell) failed.";
            return false;
        }

        Folder* zipFolder = nullptr;
        Folder* srcFolder = nullptr;
        FolderItems* items = nullptr;

        VARIANT vZip; VariantInit(&vZip);
        vZip.vt = VT_BSTR;
        vZip.bstrVal = SysAllocString(zipPath.wstring().c_str());

        VARIANT vSrc; VariantInit(&vSrc);
        vSrc.vt = VT_BSTR;
        vSrc.bstrVal = SysAllocString(sourceDir.wstring().c_str());

        hr = shell->NameSpace(vZip, &zipFolder);
        if (FAILED(hr) || !zipFolder) {
            if (errMsg) *errMsg = "Shell NameSpace(zip) failed.";
            VariantClear(&vZip); VariantClear(&vSrc);
            SafeRelease(shell);
            return false;
        }

        hr = shell->NameSpace(vSrc, &srcFolder);
        if (FAILED(hr) || !srcFolder) {
            if (errMsg) *errMsg = "Shell NameSpace(source) failed.";
            VariantClear(&vZip); VariantClear(&vSrc);
            SafeRelease(zipFolder);
            SafeRelease(shell);
            return false;
        }

        hr = srcFolder->Items(&items);
        if (FAILED(hr) || !items) {
            if (errMsg) *errMsg = "Folder->Items failed.";
            VariantClear(&vZip); VariantClear(&vSrc);
            SafeRelease(srcFolder);
            SafeRelease(zipFolder);
            SafeRelease(shell);
            return false;
        }

        // CopyHere(items, options)
        VARIANT vItems; VariantInit(&vItems);
        vItems.vt = VT_DISPATCH;
        vItems.pdispVal = items;
        items->AddRef();

        VARIANT vOpt; VariantInit(&vOpt);
        vOpt.vt = VT_I4;
        vOpt.lVal = 0x0414; // silent/no ui

        hr = zipFolder->CopyHere(vItems, vOpt);
        VariantClear(&vItems);
        VariantClear(&vOpt);

        VariantClear(&vZip);
        VariantClear(&vSrc);

        SafeRelease(items);
        SafeRelease(srcFolder);
        SafeRelease(zipFolder);
        SafeRelease(shell);

        if (FAILED(hr)) {
            if (errMsg) *errMsg = "CopyHere failed.";
            return false;
        }

        // 等待压缩完成
        auto lastSize = std::filesystem::file_size(zipPath);
        for (int i = 0; i < 50; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto sz = std::filesystem::file_size(zipPath);
            if (sz == lastSize) break;
            lastSize = sz;
        }

        return true;
    }
    catch (const std::exception& ex) {
        if (errMsg) *errMsg = ex.what();
        return false;
    }
    catch (...) {
        if (errMsg) *errMsg = "Unknown error.";
        return false;
    }
}