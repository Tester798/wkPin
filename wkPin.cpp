#include <iostream>
#include "patches.h"
#include "wkPin.h"

#pragma comment (lib, "version.lib")
#pragma comment (lib, "user32.lib")

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define WA_FILE_SIZE       3235840
#define WA_FILE_SIZE_STEAM 3231744
#define VV1 3
#define VV2 7
#define VV3 2
#define VV4 1
#define DLL_NAME "wkPin.dll"
#define WRONG_VERSION_MESSAGE "The version of your game is not " STR(VV1) "." STR(VV2) "." STR(VV3) "." STR(VV4)
#define WRONG_FILE_SIZE_MESSAGE "The size of WA.exe is not " STR(WA_FILE_SIZE)

bool is_STEAM_exe = false;

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

const char join_messagee[] = "Player joined";
char* log_message;


long GetFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


BOOL CheckWAVer(void)
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
            return 0;
        }
        GetFileVersionInfo(WApath, 0, Size, Buf);
        VS_FIXEDFILEINFO* Info;
        DWORD Is;
        if (VerQueryValue(Buf, "\\", (LPVOID*)&Info, (PUINT)&Is))
        {
            if (Info->dwSignature == 0xFEEF04BD)
            {
                if (HIWORD(Info->dwFileVersionMS) == VV1
                    && LOWORD(Info->dwFileVersionMS) == VV2
                    && HIWORD(Info->dwFileVersionLS) == VV3
                    && LOWORD(Info->dwFileVersionLS) == VV4
                    )
                    return 1;
            }
        }
    }
    return 0;
}


__declspec(naked) void WriteLogOnJoinGameHook()
{
    __asm {
        mov log_message, eax

        cmp is_STEAM_exe, 1
        je steam_proc_start
        push 0x74B1D0
        push 0x45098D
        push 0x5A6B99
        jmp steam_proc_continue

    steam_proc_start:
        push 0x74A1C8
        push 0x45081D
        push 0x5A61B6

    steam_proc_continue:
        pushad
    }
#if _DEBUG
    std::cout << "WriteLogOnJoinGameHook called" << std::endl;
#endif
    if (strncmp(join_messagee, log_message, sizeof(join_messagee)) == 0) {
#if _DEBUG
        std::cout << "join_messagee found" << std::endl;
#endif
        __asm {
            push 0 // 0 = no beep, 2 = beep
            push proc_exit

            cmp is_STEAM_exe, 1
            je push_steam_proc
            push 0x4CBD50
            ret

        push_steam_proc:
            push 0x4CB290
            ret
        }
    }
    else {
#if _DEBUG
        std::cout << "join_messagee not found" << std::endl;
#endif
    }

    __asm {
    proc_exit:
        popad
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
        mov eax, dword ptr ds : [edi + 0xF36C]
        cmp eax, 0
        jne proc_exit
        inc eax

    proc_exit:
        cmp is_STEAM_exe, 1
        je steam_proc_exit
        push 0x446FDB
        ret

    steam_proc_exit:
        push 0x446ECB
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

        cmp is_STEAM_exe, 1
        je steam_exe_ret

        push 0x53BC97
        ret

    steam_exe_ret:
        push 0x53B047
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
        cmp is_STEAM_exe, 1
        je steam_exe_ret_1

        popad
        cmp dword ptr ds : [esi + 0x1B8] , 0
        push 0x53BC17
        ret

    steam_exe_ret_1:
        popad
        cmp dword ptr ds : [esi + 0x1B8] , 0
        push 0x53AFC7
        ret

    exit_cursor_should_not_lock:
        cmp is_STEAM_exe, 1
        je steam_exe_ret_2

        popad
        cmp dword ptr ds : [esi + 0x1B8] , 0
        push 0x53BC42
        ret

    steam_exe_ret_2:
        popad
        cmp dword ptr ds : [esi + 0x1B8] , 0
        push 0x53AFF2
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
        cmp is_STEAM_exe, 1
        je steam_exe_ret

        popad
        push 0x53BC86
        ret

    steam_exe_ret:
        popad
        push 0x53B036
        ret
    }
}


void DoWindowPin()
{
#if _DEBUG
    std::cout << "pin window" << std::endl;
#endif
    is_STEAM_exe ? apply_patches(patch_1_weapon_window_always_redraw_STEAM, false) : apply_patches(patch_1_weapon_window_always_redraw, false);
    if (PinWeaponMenuDoNotDim[0] == '1') {
        is_STEAM_exe ? apply_patches(patch_1_weapon_window_do_not_dim_STEAM, false) : apply_patches(patch_1_weapon_window_do_not_dim, false);
    }
    weapon_window_pinned = true;
}


void DoWindowUnpin()
{
#if _DEBUG
    std::cout << "unpin window" << std::endl;
#endif
    is_STEAM_exe ? apply_patches(patch_1_weapon_window_always_redraw_STEAM, true) : apply_patches(patch_1_weapon_window_always_redraw, true);
    if (PinWeaponMenuDoNotDim[0] == '1') {
        is_STEAM_exe ? apply_patches(patch_1_weapon_window_do_not_dim_STEAM, true) : apply_patches(patch_1_weapon_window_do_not_dim, true);
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
    tmp << (is_STEAM_exe ? "0013B040" : "0013BC90") << ":83->68\n";
    tmp << (is_STEAM_exe ? "0013B041" : "0013BC91") << ":EC->" << hook_proc_push_addr.substr(0, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013B042" : "0013BC92") << ":08->" << hook_proc_push_addr.substr(2, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013B043" : "0013BC93") << ":53->" << hook_proc_push_addr.substr(4, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013B044" : "0013BC94") << ":56->" << hook_proc_push_addr.substr(6, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013B045" : "0013BC95") << ":8B->C3\n";
    std::vector<patch_info> patch_wnd_update_hook = read_1337_text(tmp.str());


    tmp.str("");
    tmp << std::hex << (LPVOID)_byteswap_ulong((unsigned long)WndShowHook);
    hook_proc_push_addr = tmp.str();
    tmp.str("");
    tmp << ">wa.exe\n";
    tmp << (is_STEAM_exe ? "0013AFC0" : "0013BC10") << ":83->68\n";
    tmp << (is_STEAM_exe ? "0013AFC1" : "0013BC11") << ":BE->" << hook_proc_push_addr.substr(0, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013AFC2" : "0013BC12") << ":B8->" << hook_proc_push_addr.substr(2, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013AFC3" : "0013BC13") << ":01->" << hook_proc_push_addr.substr(4, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013AFC4" : "0013BC14") << ":00->" << hook_proc_push_addr.substr(6, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013AFC5" : "0013BC15") << ":00->C3\n";
    std::vector<patch_info> patch_wnd_show_hook = read_1337_text(tmp.str());


    tmp.str("");
    tmp << std::hex << (LPVOID)_byteswap_ulong((unsigned long)WndHideHook);
    hook_proc_push_addr = tmp.str();
    tmp.str("");
    tmp << ">wa.exe\n";
    tmp << (is_STEAM_exe ? "0013B031" : "0013BC81") << ":C3->68\n";
    tmp << (is_STEAM_exe ? "0013B032" : "0013BC82") << ":CC->" << hook_proc_push_addr.substr(0, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013B033" : "0013BC83") << ":CC->" << hook_proc_push_addr.substr(2, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013B034" : "0013BC84") << ":CC->" << hook_proc_push_addr.substr(4, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013B035" : "0013BC85") << ":CC->" << hook_proc_push_addr.substr(6, 2) << "\n";
    tmp << (is_STEAM_exe ? "0013B036" : "0013BC86") << ":CC->C3\n";
    std::vector<patch_info> patch_wnd_hide_hook = read_1337_text(tmp.str());


    if (verify_patches(patch_wnd_update_hook, false)
        && verify_patches(patch_wnd_show_hook, false)
        && verify_patches(patch_wnd_hide_hook, false)
        && (is_STEAM_exe ? verify_patches(patch_1_weapon_window_always_redraw_STEAM, false) : verify_patches(patch_1_weapon_window_always_redraw, false))
        && (PinWeaponMenuDoNotDim[0] == '1' ? (is_STEAM_exe ? verify_patches(patch_1_weapon_window_do_not_dim_STEAM, false) : verify_patches(patch_1_weapon_window_do_not_dim, false)) : true)
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
#endif

        char currentDir[MAX_PATH];
        char iniFile[MAX_PATH];
        char* pos;

        GetModuleFileName(GetModuleHandle(NULL), currentDir, MAX_PATH);

        if (!CheckWAVer())
        {
#if _DEBUG
            MessageBox(0, WRONG_VERSION_MESSAGE, DLL_NAME, MB_OK | MB_ICONERROR);
#endif
            return 1;
        }
        long exe_file_size = GetFileSize(currentDir);
        if (exe_file_size != WA_FILE_SIZE && exe_file_size != WA_FILE_SIZE_STEAM)
        {
#if _DEBUG
            MessageBox(0, WRONG_FILE_SIZE_MESSAGE, DLL_NAME, MB_OK | MB_ICONERROR);
#endif
            return 1;
        }
        if (exe_file_size == WA_FILE_SIZE_STEAM) {
            is_STEAM_exe = true;
        }

        GetModuleFileName(GetModuleHandle(DLL_NAME), currentDir, MAX_PATH);
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
            tmp << (is_STEAM_exe ? "00046EC5" : "00046FD5") << ":8B->68\n";
            tmp << (is_STEAM_exe ? "00046EC6" : "00046FD6") << ":87->" << hook_proc_push_addr.substr(0, 2) << "\n";
            tmp << (is_STEAM_exe ? "00046EC7" : "00046FD7") << ":6C->" << hook_proc_push_addr.substr(2, 2) << "\n";
            tmp << (is_STEAM_exe ? "00046EC8" : "00046FD8") << ":F3->" << hook_proc_push_addr.substr(4, 2) << "\n";
            tmp << (is_STEAM_exe ? "00046EC9" : "00046FD9") << ":00->" << hook_proc_push_addr.substr(6, 2) << "\n";
            tmp << (is_STEAM_exe ? "00046ECA" : "00046FDA") << ":00->C3\n";
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

            if (is_STEAM_exe ? verify_patches(patch_2_syncronize_pinned_chat_STEAM, false) : verify_patches(patch_2_syncronize_pinned_chat, false))
            {
#if _DEBUG
                std::cout << "Bytes for 'SyncPinnedAndOpenedLines' found, applying patch" << std::endl;
#endif

                is_STEAM_exe ? apply_patches(patch_2_syncronize_pinned_chat_STEAM, false) : apply_patches(patch_2_syncronize_pinned_chat, false);
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
            tmp << (is_STEAM_exe ? "00050814" : "00050984") << ":" << (is_STEAM_exe ? "C8" : "D0") << "->" << hook_proc_push_addr.substr(0, 2) << "\n";
            tmp << (is_STEAM_exe ? "00050815" : "00050985") << ":" << (is_STEAM_exe ? "A1" : "B1") << "->" << hook_proc_push_addr.substr(2, 2) << "\n";
            tmp << (is_STEAM_exe ? "00050816" : "00050986") << ":74->" << hook_proc_push_addr.substr(4, 2) << "\n";
            tmp << (is_STEAM_exe ? "00050817" : "00050987") << ":00->" << hook_proc_push_addr.substr(6, 2) << "\n";
            tmp << (is_STEAM_exe ? "00050818" : "00050988") << ":E8->C3\n";
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