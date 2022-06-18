#include "includes.h"

#pragma pack(push, 1)
struct JMP_SHELLCODE {
private:
    WORD _mov = 0xB848;
    PVOID _dst;
    WORD _jmp = 0xE0FF;
public:
    JMP_SHELLCODE(PVOID dst, WORD jmp = 0xE0FF, WORD mov = 0xB848) {
        _dst = dst;
        _jmp = jmp;
        _mov = mov;
    }
};
#pragma pack(pop)

#pragma pack(push, 1)
struct Pit
{
private:
    SaveAllRegistersX64;
    SaveAllFlagsX64;
    WORD _mov = 0xBF48;
    PVOID _paintFunction;
    WORD _jmp_rax = 0xD7FF;
    RestoreAllFlagsX64;
    RestoreAllRegistersX64;
    byte _callOpcode = 0xE9;
    byte _retFunction[4]{ 0 };
public:
    Pit(PVOID paintFunction) {
        _paintFunction = paintFunction;
    }
};
#pragma pack(pop)

void* hook::GetPointerEndScene()
{
    char m[] = { 0x20, 0x30, 0x40 };
    IDirect3DDevice9* fakeDevise;
    IDirect3D9* p_vmt = Direct3DCreate9(D3D_SDK_VERSION);
    D3DPRESENT_PARAMETERS d3dParams;
    ZeroMemory(&d3dParams, sizeof(d3dParams));
    d3dParams.Windowed = true;
    d3dParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dParams.hDeviceWindow = GetForegroundWindow();
    p_vmt->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dParams.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dParams, &fakeDevise);
    void* p_endScene = *(*(void***)fakeDevise + 0x2A);
    fakeDevise->Release();
    p_vmt->Release();   
    return p_endScene;
}

void hook::InstallHook(void* p_endScene)
{
    DWORD oldProtect;
    VirtualProtect(p_endScene, 15, PAGE_EXECUTE_READWRITE, &oldProtect);
    PVOID ret = VirtualAlloc(NULL, 27, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memcpy(ret, p_endScene, 15);
    JMP_SHELLCODE retJmp((PVOID)((__int64)(p_endScene)+15), 0xE3FF, 0xBB48);
    memcpy(PVOID((__int64)ret + 15), &retJmp, sizeof(JMP_SHELLCODE));
    Pit pit((LPVOID)((uintptr_t)draftsman::Paint+5));
    PVOID adr = VirtualAlloc(NULL, sizeof(pit), MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    memcpy(adr, &pit, sizeof(pit));
    *(DWORD*)((__int64)(adr)+63) = (__int64)ret - ((__int64)(adr)+62) - 5;
    JMP_SHELLCODE trumpPit(adr);
    memcpy(p_endScene, &trumpPit, 12);
    memset((LPVOID)((uintptr_t)p_endScene + 12), 0x90, 3);
    VirtualProtect(p_endScene, 15, oldProtect, &oldProtect);
}

