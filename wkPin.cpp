#include <iostream>
#include "patches.h"
#include "wkPin.h"

#pragma comment (lib, "version.lib")
#pragma comment (lib, "user32.lib")

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define WA_FILE_SIZE 3596288
const std::string DLL_NAME = "wkPin.dll";
const std::string ALLOWED_VER = "3.8.0.0";

char SyncPinnedAndOpenedLines[2];
char PinWeaponMenuEnable[2];
char PinWeaponMenuAtStart[2];
char PinWeaponMenuDoNotDim[2];
char FlashWindowWhenUserJoinsGame[2];

bool hook_was_called = false;

bool weapon_window_opened = false;
bool weapon_window_pinned = false;
bool weapon_cursor_locked = false;

#if _DEBUG
bool old_weapon_window_opened = false;
bool old_weapon_window_pinned = false;
bool old_weapon_cursor_locked = false;
#endif

bool* window_show_addr;
bool* window_show_addr_old;

const char join_message[] = "Player joined";
char* log_message;


long GetFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


bool CheckWAVer(char* currentDir)
{
    char WApath[MAX_PATH];
    DWORD h;
    GetModuleFileName(0, WApath, MAX_PATH);
    DWORD Size = GetFileVersionInfoSize(WApath, &h);
    if (Size)
    {
        void* Buf = malloc(Size);
        if (Buf == NULL) {
#if _DEBUG
            std::cout << "Can not allocate memory" << std::endl;
#endif
            return false;
        }
        GetFileVersionInfo(WApath, 0, Size, Buf);
        VS_FIXEDFILEINFO* Info;
        DWORD Is;
        if (VerQueryValue(Buf, "\\", (LPVOID*)&Info, (PUINT)&Is))
        {
            if (Info->dwSignature == 0xFEEF04BD)
            {
                long exe_file_size = GetFileSize(currentDir);

                std::stringstream tmp;
                tmp.str("");
                tmp << HIWORD(Info->dwFileVersionMS) << "." << LOWORD(Info->dwFileVersionMS) << "." << HIWORD(Info->dwFileVersionLS) << "." << LOWORD(Info->dwFileVersionLS);
                std::string ver_file = tmp.str();
#if _DEBUG
                std::cout << "Detected WA.exe version: " << ver_file << std::endl;
#endif
                if (ver_file == ALLOWED_VER) {
                    if (exe_file_size != WA_FILE_SIZE) {
#if _DEBUG
                        MessageBox(0, "The size of WA.exe is not " STR(WA_FILE_SIZE), DLL_NAME.c_str(), MB_OK | MB_ICONERROR);
#endif
                        return false;
                    }
                    return true;
                }
#if _DEBUG
                tmp.str("");
                tmp << "The version of WA.exe is not " << ALLOWED_VER;
                MessageBox(0, tmp.str().data(), DLL_NAME.c_str(), MB_OK | MB_ICONERROR);
#endif
            }
        }
    }
    return false;
}


__declspec(naked) void WriteLogOnJoinGameHook()
{
    __asm {
        mov log_message, eax
        pushad
    }
#if _DEBUG
    std::cout << "WriteLogOnJoinGameHook called" << std::endl;
#endif

    if (strncmp(join_message, log_message, sizeof(join_message)) == 0) {
#if _DEBUG
        std::cout << "join_message found" << std::endl;
#endif
        __asm {
            push 0 // 0 = no beep, 2 = beep
            push proc_exit

            mov eax, moduleBase
            add eax, 0xEDCD0
            push eax
            ret
        }
    }
    else {
#if _DEBUG
        std::cout << "join_message not found" << std::endl;
#endif
    }

    __asm {
    proc_exit:
        popad
        push 0xffffffff
        push 0x3fff

        mov eax, moduleBase
        add eax, 0x6AB43
        push eax

        ret
    }
}


__declspec(naked) void FixZeroPinnedChatLinesHook()
{
#if _DEBUG
    __asm {
        pushad
    }
    std::cout << "FixZeroPinnedChatLinesProc called" << std::endl;
    __asm {
        popad
    }
#endif
    __asm {
        mov ecx, dword ptr ds : [edi + 0xF3AC]
        cmp ecx, 0
        jne proc_exit
        inc ecx

    proc_exit:
        push ecx

        mov ecx, moduleBase
        add ecx, 0x60e98
        push ecx
        ret
    }
}


__declspec(naked) void WndUpdateHook()
{
    __asm {
        pushad

        mov window_show_addr, ecx
        add window_show_addr, 0x1da

        // reset "hook_was_called" if new game started
        cmp ecx, window_show_addr_old
        je skip_set
        mov hook_was_called, 0
        skip_set:
        mov window_show_addr_old, ecx
    }

    if (!hook_was_called) {
        hook_was_called = true;

        // reset vars on new game
        weapon_window_opened = false;
        weapon_window_pinned = false;
        weapon_cursor_locked = false;

        if (PinWeaponMenuAtStart[0] == '1') {
            DoWindowPin();
            DoWindowShow(true);
        }
#if _DEBUG
        std::cout << "Hook WndUpdateHook called on new game" << std::endl;
        std::cout << "window_show_addr: " << std::hex << (uintptr_t)window_show_addr << std::endl;
#endif
    }

    __asm {
        popad

        sub esp, 8
        push ebx
        push esi
        mov esi, ecx
        push edi

        mov edi, moduleBase
        add edi, 0x169348
        push edi
        ret
    }
}


__declspec(naked) void WndShowHook()
{
    _asm {
        pushad
    }

#if _DEBUG
    std::cout << "Hook wnd SHOW hook called" << std::endl;
#endif
    WndShowHideHookProc();

    __asm {
        cmp weapon_cursor_locked, 1
        je exit_cursor_should_lock
        jmp exit_cursor_should_not_lock

    exit_cursor_should_lock:
        popad
        cmp dword ptr ds : [esi + 0x1B8] , 0
        mov dword ptr ds : [esi + 0x1DC] , 0x1
        jge do_jmp

        mov ecx, moduleBase
        add ecx, 0x1692D3
        push ecx
        ret

    do_jmp:
        mov ecx, moduleBase
        add ecx, 0x1692E8
        push ecx
        ret

    exit_cursor_should_not_lock:
        popad

        mov ecx, moduleBase
        add ecx, 0x1692F2
        push ecx
        ret
    }
}


__declspec(naked) void WndHideHook()
{
    _asm {
        pushad
    }

#if _DEBUG
    std::cout << "Hook wnd HIDE hook called" << std::endl;
#endif
    WndShowHideHookProc();

    __asm {
        popad

        mov eax, moduleBase
        add eax, 0x169336
        push eax
        ret
    }
}


void DoWindowPin()
{
#if _DEBUG
    std::cout << "pin window" << std::endl;
#endif
    apply_patches(patch_1_weapon_window_always_redraw, false);
    if (PinWeaponMenuDoNotDim[0] == '1') {
        apply_patches(patch_1_weapon_window_do_not_dim, false);
    }
    weapon_window_pinned = true;
}


void DoWindowUnpin()
{
#if _DEBUG
    std::cout << "unpin window" << std::endl;
#endif
    apply_patches(patch_1_weapon_window_always_redraw, true);
    if (PinWeaponMenuDoNotDim[0] == '1') {
        apply_patches(patch_1_weapon_window_do_not_dim, true);
    }
    weapon_window_pinned = false;
}


void DoWindowShow(bool ignore_check = false)
{
#if _DEBUG
    std::cout << "show window" << std::endl;
#endif
    if (!ignore_check && !hook_was_called) return;
    _asm {
        mov esi, window_show_addr
        mov byte ptr ds : [esi] , 1
    }
    weapon_window_opened = true;
}


void DoWindowHide(bool ignore_check = false)
{
#if _DEBUG
    std::cout << "hide window" << std::endl;
#endif
    if (!ignore_check && !hook_was_called) return;
    _asm {
        mov esi, window_show_addr
        mov byte ptr ds : [esi] , 0
    }
    weapon_window_opened = false;
}


void LockCursorToWindow(bool ignore_check = false)
{
#if _DEBUG
    std::cout << "lock cursor" << std::endl;
#endif
    if (!ignore_check && !hook_was_called) return;
    _asm {
        mov esi, window_show_addr
        mov byte ptr ds : [esi + 2] , 1
    }
    weapon_cursor_locked = true;
}


void UnlockCursorToWindow(bool ignore_check = false)
{
#if _DEBUG
    std::cout << "unlock cursor" << std::endl;
#endif
    if (!ignore_check && !hook_was_called) return;
    _asm {
        mov esi, window_show_addr
        mov byte ptr ds : [esi + 2] , 0
    }
    weapon_cursor_locked = false;
}


void WndShowHideHookProc()
{
    do {
        if (GetKeyState(VK_CONTROL) & 0x8000)
        {
#if _DEBUG
            std::cout << "<control + rmb> pressed" << std::endl;
#endif
            if (weapon_window_pinned)
            {
                DoWindowUnpin();
                DoWindowHide();
                UnlockCursorToWindow();
            }
            else {
                DoWindowPin();
                DoWindowShow();
                if (weapon_cursor_locked) UnlockCursorToWindow();
            }
            break;
        }

#if _DEBUG
        std::cout << "<rmb> pressed" << std::endl;
#endif
        if (weapon_cursor_locked) {
            if (!weapon_window_pinned) DoWindowHide();
            UnlockCursorToWindow();
        }
        else {
            if (!weapon_window_pinned) DoWindowShow();
            LockCursorToWindow();
        }
    } while (false);

    if (weapon_window_opened) {
        _asm {
            mov esi, window_show_addr
            mov byte ptr ds : [esi] , 1
        }
    }
    else {
        _asm {
            mov esi, window_show_addr
            mov byte ptr ds : [esi] , 0
        }

    }

    if (weapon_cursor_locked) {
        _asm {
            mov esi, window_show_addr
            mov byte ptr ds : [esi + 2] , 1
        }
    }
    else {
        _asm {
            mov esi, window_show_addr
            mov byte ptr ds : [esi + 2] , 0
        }
    }
}


BOOL WINAPI ThreadLoop(HMODULE hModule)
{
    std::stringstream tmp;

    tmp.str("");
    tmp << std::hex << (LPVOID)_byteswap_ulong((unsigned long)WndUpdateHook);
    std::string hook_proc_push_addr = tmp.str();
    tmp.str("");
    tmp << ">wa.exe\n";
    tmp << "00169340:83->68\n";
    tmp << "00169341:EC->" << hook_proc_push_addr.substr(0, 2) << "\n";
    tmp << "00169342:08->" << hook_proc_push_addr.substr(2, 2) << "\n";
    tmp << "00169343:53->" << hook_proc_push_addr.substr(4, 2) << "\n";
    tmp << "00169344:56->" << hook_proc_push_addr.substr(6, 2) << "\n";
    tmp << "00169345:8B->C3\n";
    tmp << "00169346:F1->90\n";
    std::vector<patch_info> patch_wnd_update_hook = read_1337_text(tmp.str());


    tmp.str("");
    tmp << std::hex << (LPVOID)_byteswap_ulong((unsigned long)WndShowHook);
    hook_proc_push_addr = tmp.str();
    tmp.str("");
    tmp << ">wa.exe\n";
    tmp << "001692C0:83->68\n";
    tmp << "001692C1:BE->" << hook_proc_push_addr.substr(0, 2) << "\n";
    tmp << "001692C2:B8->" << hook_proc_push_addr.substr(2, 2) << "\n";
    tmp << "001692C3:01->" << hook_proc_push_addr.substr(4, 2) << "\n";
    tmp << "001692C4:00->" << hook_proc_push_addr.substr(6, 2) << "\n";
    tmp << "001692C5:00->C3\n";
    tmp << "001692C6:00->90\n";
    std::vector<patch_info> patch_wnd_show_hook = read_1337_text(tmp.str());


    tmp.str("");
    tmp << std::hex << (LPVOID)_byteswap_ulong((unsigned long)WndHideHook);
    hook_proc_push_addr = tmp.str();
    tmp.str("");
    tmp << ">wa.exe\n";
    tmp << "00169331:C3->68\n";
    tmp << "00169332:CC->" << hook_proc_push_addr.substr(0, 2) << "\n";
    tmp << "00169333:CC->" << hook_proc_push_addr.substr(2, 2) << "\n";
    tmp << "00169334:CC->" << hook_proc_push_addr.substr(4, 2) << "\n";
    tmp << "00169335:CC->" << hook_proc_push_addr.substr(6, 2) << "\n";
    tmp << "00169336:CC->C3\n";
    std::vector<patch_info> patch_wnd_hide_hook = read_1337_text(tmp.str());


    if (verify_patches(patch_wnd_update_hook, false)
        && verify_patches(patch_wnd_show_hook, false)
        && verify_patches(patch_wnd_hide_hook, false)
        && verify_patches(patch_1_weapon_window_always_redraw, false)
        && (PinWeaponMenuDoNotDim[0] == '1' ? verify_patches(patch_1_weapon_window_do_not_dim, false) : true)
        )
    {
#if _DEBUG
        std::cout << "Bytes for hooks found, applying patch" << std::endl;
#endif
        apply_patches(patch_wnd_update_hook, false);
        apply_patches(patch_wnd_show_hook, false);
        apply_patches(patch_wnd_hide_hook, false);
    }
    else {
#if _DEBUG
        std::cout << "Cannot find original bytes for hooks" << std::endl;
#endif
        return 1;
    }

#if _DEBUG
    while (true)
    {
        Sleep(1000);

        if (old_weapon_window_opened != weapon_window_opened
            || old_weapon_window_pinned != weapon_window_pinned
            || old_weapon_cursor_locked != weapon_cursor_locked
            )
        {
            std::cout << "-----------------------" << std::endl;
            std::cout << "weapon_window_opened: " << weapon_window_opened << std::endl;
            std::cout << "weapon_window_pinned: " << weapon_window_pinned << std::endl;
            std::cout << "weapon_cursor_locked: " << weapon_cursor_locked << std::endl;
            std::cout << "-----------------------" << std::endl;

            old_weapon_window_opened = weapon_window_opened;
            old_weapon_window_pinned = weapon_window_pinned;
            old_weapon_cursor_locked = weapon_cursor_locked;
        }
    }
#endif

    return 1;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {

#if _DEBUG
        AllocConsole();
        FILE* f;
        std::string console_name = DLL_NAME;
        console_name += " console window";
        freopen_s(&f, "CONOUT$", "w", stdout);
        SetConsoleTitle(console_name.c_str());
        system("cls");
        std::cout << "Started" << std::endl;
        std::cout << "WA.exe moduleBase: " << std::hex << moduleBase << std::endl;
#endif

        char currentDir[MAX_PATH];
        char iniFile[MAX_PATH];
        char* pos;

        GetModuleFileName(GetModuleHandle(NULL), currentDir, MAX_PATH);

        if (!CheckWAVer(currentDir))
        {
            return 1;
        }

        GetModuleFileName(GetModuleHandle(DLL_NAME.c_str()), currentDir, MAX_PATH);
        pos = strrchr(currentDir, '.');
        strcpy(pos, "");
        strcpy(iniFile, currentDir);
        strcat(iniFile, ".ini");

        GetPrivateProfileString("Settings", "SyncPinnedAndOpenedLines", "1", SyncPinnedAndOpenedLines, 2, iniFile);
        GetPrivateProfileString("Settings", "PinWeaponMenuEnable", "1", PinWeaponMenuEnable, 2, iniFile);
        GetPrivateProfileString("Settings", "PinWeaponMenuAtStart", "0", PinWeaponMenuAtStart, 2, iniFile);
        GetPrivateProfileString("Settings", "PinWeaponMenuDoNotDim", "0", PinWeaponMenuDoNotDim, 2, iniFile);
        GetPrivateProfileString("Settings", "FlashWindowWhenUserJoinsGame", "1", FlashWindowWhenUserJoinsGame, 2, iniFile);

        WritePrivateProfileString("Settings", "SyncPinnedAndOpenedLines", SyncPinnedAndOpenedLines, iniFile);
        WritePrivateProfileString("Settings", "PinWeaponMenuEnable", PinWeaponMenuEnable, iniFile);
        WritePrivateProfileString("Settings", "PinWeaponMenuAtStart", PinWeaponMenuAtStart, iniFile);
        WritePrivateProfileString("Settings", "PinWeaponMenuDoNotDim", PinWeaponMenuDoNotDim, iniFile);
        WritePrivateProfileString("Settings", "FlashWindowWhenUserJoinsGame", FlashWindowWhenUserJoinsGame, iniFile);

        if (SyncPinnedAndOpenedLines[0] == '1') {
            // fixing WA bug with resetting pinned chat lines if number of pinned lines is 1
            std::stringstream tmp;
            tmp.str("");
            tmp << std::hex << (LPVOID)_byteswap_ulong((unsigned long)FixZeroPinnedChatLinesHook);
            std::string hook_proc_push_addr = tmp.str();
            tmp.str("");
            tmp << ">wa.exe\n";
            tmp << "00060E91" << ":8B->68\n";
            tmp << "00060E92" << ":8F->" << hook_proc_push_addr.substr(0, 2) << "\n";
            tmp << "00060E93" << ":AC->" << hook_proc_push_addr.substr(2, 2) << "\n";
            tmp << "00060E94" << ":F3->" << hook_proc_push_addr.substr(4, 2) << "\n";
            tmp << "00060E95" << ":00->" << hook_proc_push_addr.substr(6, 2) << "\n";
            tmp << "00060E96" << ":00->C3\n";
            std::vector<patch_info> patch_fix_pinned_lines = read_1337_text(tmp.str());
            if (verify_patches(patch_fix_pinned_lines, false))
            {
#if _DEBUG
                std::cout << "Bytes for 'patch_fix_pinned_lines' found, applying patch" << std::endl;
#endif
                apply_patches(patch_fix_pinned_lines, false);
            }
            else {
#if _DEBUG
                std::cout << "Cannot find original bytes for 'patch_fix_pinned_lines'" << std::endl;
#endif
            }

            if (verify_patches(patch_2_syncronize_pinned_chat, false))
            {
#if _DEBUG
                std::cout << "Bytes for 'SyncPinnedAndOpenedLines' found, applying patch" << std::endl;
#endif

                apply_patches(patch_2_syncronize_pinned_chat, false);
            }
            else {
#if _DEBUG
                std::cout << "Cannot find original bytes for 'SyncPinnedAndOpenedLines'" << std::endl;
#endif
            }
        }

        if (FlashWindowWhenUserJoinsGame[0] == '1') {
            // flash window on task bar when new player joins game
            std::stringstream tmp;
            tmp.str("");
            tmp << std::hex << (LPVOID)_byteswap_ulong((unsigned long)WriteLogOnJoinGameHook);
            std::string hook_proc_push_addr = tmp.str();
            tmp.str("");
            tmp << ">wa.exe\n";
            tmp << "0006AB3C:6A->68" << "\n";
            tmp << "0006AB3D:FF->" << hook_proc_push_addr.substr(0, 2) << "\n";
            tmp << "0006AB3E:68->" << hook_proc_push_addr.substr(2, 2) << "\n";
            tmp << "0006AB3F:FF->" << hook_proc_push_addr.substr(4, 2) << "\n";
            tmp << "0006AB40:3F->" << hook_proc_push_addr.substr(6, 2) << "\n";
            tmp << "0006AB41:00->C3\n";
            tmp << "0006AB42:00->90\n";
            std::vector<patch_info> patch_flash_window_on_join = read_1337_text(tmp.str());
            if (verify_patches(patch_flash_window_on_join, false))
            {
#if _DEBUG
                std::cout << "Bytes for 'patch_flash_window_on_join' found, applying patch" << std::endl;
#endif
                apply_patches(patch_flash_window_on_join, false);
            }
            else {
#if _DEBUG
                std::cout << "Cannot find original bytes for 'patch_flash_window_on_join'" << std::endl;
#endif
            }
        }

        if (PinWeaponMenuEnable[0] == '1') {
            CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadLoop, 0, 0, 0);
        }
    }

    return 1;
}