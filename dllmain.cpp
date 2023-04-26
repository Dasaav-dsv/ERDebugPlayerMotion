#include "dllmain.h"

// A multiplier to be applied to each axis of the player's motion.
// Leave it at { 1.0f, 1.0f, 1.0f, 1.0f } if you don't want to additionally influence the player motion,
// beyond what the debug motion TAE already does.
const V4D motionMulXZY{ 1.0f, 1.0f, 1.0f, 1.0f };

void debugPlayerMotion(void* CSBehCharaProxyDriver, void* hkbCharacter, void** hkxAnim, unsigned char** pFrameData)
{
    // The ChrIns pointer is at offset 0xB0 of the CSBehCharaProxyDriver struct.
    void* ChrIns = *PointerChain::make<void*>(CSBehCharaProxyDriver, 0xB0);
    // Check if CSBehCharaProxyDriver is missing the ChrIns pointer, just in case.
    if (!ChrIns) return;

    // Call the 36th ChrIns virtual function - it returns 1 if the ChrIns is a player.
    // Remove this check if you want to unlock debug motion for all characters, not just players.
    using IsPlayerFn = unsigned char (*)();
    if ((*reinterpret_cast<IsPlayerFn**>(ChrIns))[35]() != 1) return;

    // Get the current debug motion multipliers from the CSChrActionFlagModule,
    // check if dereferencing encounters a nullptr pointer at any of the offsets.
    auto pDebugMotionMul = PointerChain::make<V4D>(ChrIns, 0x190, 0x8u, 0x1F0u).get();
    if (!pDebugMotionMul) return;
    V4D debugMotionMul = *pDebugMotionMul;

    // Get the current character motion vector from the frame data.
    auto pCurrMotion = PointerChain::make<V4D>(pFrameData, *reinterpret_cast<int*>(*pFrameData + 0x14)).get();
    V4D currMotion = *pCurrMotion;

    // Multiply the current motion vector first with debug and then with our multipliers,
    // using the correct component order for the debug motion mulitpliers (don't ask me about it, it's fromsoft)
    currMotion = _mm_mul_ps(currMotion, V4D(debugMotionMul[2], debugMotionMul[3], debugMotionMul[1], 1.0f));
    currMotion = _mm_mul_ps(currMotion, motionMulXZY);

    // Write back the modified motion values.
    *pCurrMotion = currMotion;
}

void onAttach() 
{
    // Create an RTTI scanner instance to find the virtual function table we want to hook.
    RTTIScanner* scanner = new RTTIScanner();

    if (scanner->scan()) {
        // Hook the 6th CS::CSBehCharaProxyDriver virtual function table with our function.
        new VFTHook("CS::CSBehCharaProxyDriver", 5, debugPlayerMotion);
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        // Do our stuff on DLL load.
        onAttach();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

