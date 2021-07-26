#pragma once

#include "patching.h"


// allow float timer
std::vector<patch_info> patch_allow_float_timer = read_1337_text(R"(
>wa.exe
0015E8E6:75->EB
0015E905:74->90
0015E906:1A->90
0015EE3E:74->90
0015EE3F:53->90
0015EF98:74->90
0015EF99:5E->90
0015F013:0F->90
0015F014:84->90
0015F015:E6->90
0015F016:01->90
0015F017:00->90
0015F018:00->90
0015F02E:74->90
0015F02F:0E->90
0015F0CB:0F->90
0015F0CC:84->90
0015F0CD:DF->90
0015F0CE:00->90
0015F0CF:00->90
0015F0D0:00->90
0015F2A7:74->90
0015F2A8:1F->90
)");

// make weapon window always redraw
std::vector<patch_info> patch_1_weapon_window_always_redraw = read_1337_text(R"(
>wa.exe
00169804:74->90
00169805:54->90
)");

// skip weapon window dimming on turn end
std::vector<patch_info> patch_1_weapon_window_do_not_dim = read_1337_text(R"(
>wa.exe
001694F0:80->C3
)");

// syncronize pinned chat lines with opened chat lines
std::vector<patch_info> patch_2_syncronize_pinned_chat = read_1337_text(R"(
>wa.exe
0012CA65:7D->0F
0012CA66:05->8D
0012CA67:83->7F
0012CA68:C1->00
0012CA69:01->00
0012CA6A:89->00
0012CA6B:08->90
0012CA6C:83->90
0012CA6D:BE->90
0012CA6E:10->90
0012CA6F:04->90
0012CA70:00->90
0012CA71:00->90
0012CA72:00->90
0012CA73:C7->90
0012CA74:86->90
0012CA75:04->90
0012CA76:04->90
0012CA77:00->90
0012CA78:00->90
0012CA79:01->90
0012CA7A:00->41
0012CA7B:00->89
0012CA7C:00->8E
0012CA7D:75->A0
0012CA7E:14->02
0012CA7F:8B->00
0012CA80:86->00
0012CA81:A4->89
0012CA82:02->8E
0012CA83:00->A4
0012CA84:00->02
0012CA85:39->00
0012CA86:86->00
0012CA87:A0->C7
0012CA88:02->86
0012CA89:00->04
0012CA8A:00->04
0012CA8B:7D->00
0012CA8C:06->00
0012CA8D:89->01
0012CA8E:86->00
0012CA8F:A0->00
0012CA90:02->00
0012CA91:00->EB
0012CA92:00->57
0012CAB6:3B->83
0012CAB7:8E->F9
0012CAB8:A8->01
0012CAB9:02->7E
0012CABA:00->2F
0012CABB:00->90
0012CABC:7E->90
0012CABD:05->90
0012CABE:83->90
0012CABF:C1->90
0012CAC0:FF->90
0012CAC1:89->90
0012CAC2:08->90
0012CAC3:83->90
0012CAC4:BE->90
0012CAC5:10->90
0012CAC6:04->90
0012CAC7:00->90
0012CAC8:00->90
0012CAC9:00->90
0012CACA:C7->90
0012CACB:86->90
0012CACC:04->90
0012CACD:04->90
0012CACE:00->90
0012CACF:00->90
0012CAD0:01->90
0012CAD1:00->90
0012CAD2:00->90
0012CAD3:00->49
0012CAD4:74->89
0012CAD5:14->8E
0012CAD6:8B->A0
0012CAD7:86->02
0012CAD8:A0->00
0012CAD9:02->00
0012CADA:00->89
0012CADB:00->8E
0012CADC:39->A4
0012CADD:86->02
0012CADE:A4->00
0012CADF:02->00
0012CAE0:00->C7
0012CAE1:00->86
0012CAE2:7E->04
0012CAE3:06->04
0012CAE4:89->00
0012CAE5:86->00
0012CAE6:A4->01
0012CAE7:02->00
)");