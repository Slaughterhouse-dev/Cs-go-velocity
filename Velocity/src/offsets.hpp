#pragma once
#include <cstdint>

// Оффсеты из hazedumper (2023-09-05)
// https://github.com/frk1/hazedumper

namespace hazedumper {

namespace netvars {
    constexpr std::ptrdiff_t m_vecVelocity = 0x114;
    constexpr std::ptrdiff_t m_vecOrigin = 0x138;
    constexpr std::ptrdiff_t m_iHealth = 0x100;
    constexpr std::ptrdiff_t m_fFlags = 0x104;
    constexpr std::ptrdiff_t m_iTeamNum = 0xF4;
    constexpr std::ptrdiff_t m_bSpotted = 0x93D;
    constexpr std::ptrdiff_t m_ArmorValue = 0x117CC;
    constexpr std::ptrdiff_t m_bHasHelmet = 0x117C0;
    constexpr std::ptrdiff_t m_bIsScoped = 0x9974;
    constexpr std::ptrdiff_t m_aimPunchAngle = 0x303C;
    constexpr std::ptrdiff_t m_viewPunchAngle = 0x3030;
    constexpr std::ptrdiff_t m_angEyeAnglesX = 0x117D0;
    constexpr std::ptrdiff_t m_angEyeAnglesY = 0x117D4;
    constexpr std::ptrdiff_t m_dwBoneMatrix = 0x26A8;
    constexpr std::ptrdiff_t m_iCrosshairId = 0x11838;
    constexpr std::ptrdiff_t m_iGlowIndex = 0x10488;
    constexpr std::ptrdiff_t m_flFlashDuration = 0x10470;
    constexpr std::ptrdiff_t m_hActiveWeapon = 0x2F08;
    constexpr std::ptrdiff_t m_iShotsFired = 0x103E0;
}

namespace signatures {
    constexpr std::ptrdiff_t dwLocalPlayer = 0xDEB99C;
    constexpr std::ptrdiff_t dwEntityList = 0x4E0102C;
    constexpr std::ptrdiff_t dwClientState = 0x59F19C;
    constexpr std::ptrdiff_t dwClientState_GetLocalPlayer = 0x180;
    constexpr std::ptrdiff_t dwClientState_ViewAngles = 0x4D90;
    constexpr std::ptrdiff_t dwGlowObjectManager = 0x535BAD0;
    constexpr std::ptrdiff_t dwForceJump = 0x52BCD88;
    constexpr std::ptrdiff_t dwForceAttack = 0x322EE98;
    constexpr std::ptrdiff_t dwViewMatrix = 0x4DF1E74;
    constexpr std::ptrdiff_t dwGlobalVars = 0x59EE60;
    constexpr std::ptrdiff_t dwInput = 0x525E600;
    constexpr std::ptrdiff_t m_bDormant = 0xED;
}

} // namespace hazedumper
