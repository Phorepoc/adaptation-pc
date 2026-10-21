// Copyright The Phorepoc Authors
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later

// phesvgatest.cpp -- OURS (vm-gpu-options.md S2): drives the SVGA3D kernel driver through RVmwSvgaChannel. What vmwgfx's kernel and the Mesa svga winsys do, in
// the smallest form that shows pixels on the host: object tables -> MOB -> guest-backed surface -> screen target -> (CPU image) -> DX context + COTables ->
// render target view -> GPU clear -> present. Command layouts are svga3d_cmd.h / svga3d_dx.h (Mesa, MIT). Prints "PH: svga" lines.
#include <e32base.h>
#include <e32debug.h>
#include "rvmwsvgachannel.h"

enum
	{
	KCmdDefineGbSurface = 1097, KCmdBindGbSurface = 1099, KCmdUpdateGbImage = 1101, KCmdSetOtableBase64 = 1115,
	KCmdDefineGbScreenTarget = 1124, KCmdDestroyGbScreenTarget = 1125, KCmdBindGbScreenTarget = 1126, KCmdUpdateGbScreenTarget = 1127,
	KCmdDefineGbMob64 = 1135, KCmdDxDefineContext = 1143, KCmdDxBindContext = 1145, KCmdDxClearRenderTargetView = 1176,
	KCmdDxDefineRenderTargetView = 1187, KCmdDxSetCoTable = 1207,
	KMobFmtRange = 3, KSurfScreenTarget = 1 << 16, KSurfBindShaderResource = 1 << 23, KSurfBindRenderTarget = 1 << 24,
	KFmtB8G8R8A8Unorm = 141, KResTexture2D = 3, KStFlagPrimary = 1, KCbFlagDx = 2,
	// the arena layout (the client's own choice; the kernel only checks bounds)
	KOffCmds = 0x1000, KOffOtables = 0x10000, KOffImage = 0x20000, KOffDxCtx = 0x100000, KDxCtxBytes = 0x10000, KOffCoTables = 0x110000,
	KNumCoTables = 12, KW = 360, KH = 320
	};

class TCmds
	{
public:
	TCmds(TUint8* aBase, TInt aOffset) : iBase(aBase), iOffset(aOffset), iLen(0) {}
	void Add(TUint32 aId, const TUint32* aBody, TInt aBytes)
		{
		TUint32* h = (TUint32*)(iBase + iOffset + iLen);
		h[0] = aId;
		h[1] = aBytes;
		Mem::Copy(h + 2, aBody, aBytes);
		iLen += 8 + aBytes;
		}
	TUint8* iBase;
	TInt iOffset, iLen;
	};

static TInt Run(RVmwSvgaChannel& aCh, TCmds& aC, TUint32 aFlags, TUint32 aDx, const char* aWhat)
	{
	TVmwSubmit s;
	Mem::FillZ(&s, sizeof(s));
	s.iOffset = aC.iOffset;
	s.iLength = aC.iLen;
	s.iContext = 0;
	s.iFlags = aFlags;
	s.iDxContext = aDx;
	const TInt r = aCh.Submit(s);
	RDebug::Printf("PH: svga %s: %d bytes -> r=%d status %u (1 = completed) error offset %u", aWhat, aC.iLen, r, s.iStatus, s.iErrorOffset);
	return (r == KErrNone && s.iStatus == 1) ? KErrNone : KErrGeneral;
	}

static TInt MainL()
	{
	RVmwSvgaChannel ch;
	TInt r = ch.Open();
	RDebug::Printf("PH: svga open %d", r);
	if (r != KErrNone)
		return r;
	TVmwSvgaInfo info;
	r = ch.GetInfo(info);
	RDebug::Printf("PH: svga info r=%d id %d caps %08x cap2 %08x arena %08x %d KB devcaps 3D %u DX %u SM41 %u SM5 %u GL43 %u maxtex %u", r, info.iVersion, info.iCaps,
		info.iCap2, info.iArenaPhys, info.iArenaSize >> 10, info.iDevCaps[0], info.iDevCaps[1], info.iDevCaps[2], info.iDevCaps[3], info.iDevCaps[4], info.iDevCaps[5]);
	RChunk arena;
	r = ch.OpenArena(arena);
	RDebug::Printf("PH: svga arena chunk r=%d base %08x size %d", r, (TUint)arena.Base(), arena.Size());
	if (r != KErrNone)
		return r;
	TUint8* base = arena.Base();
	const TUint32 phys = info.iArenaPhys;
	Mem::FillZ(base, 0x20000);						// cmds + otables
	// ---- 1: tables, MOB, surface, screen target, the CPU image ----
	{
	TCmds c(base, KOffCmds);
	static const TUint32 KOtables[] = { 0, 1, 2, 3, 4, 5 };	// MOB, surface, context, shader, screen target, DX context
	for (TUint i = 0; i < 6; ++i)
		{
		const TUint32 ppn = (phys + KOffOtables + i * 0x1000) >> 12;
		const TUint32 b[6] = { KOtables[i], ppn, 0, 0x1000, 0, KMobFmtRange };
		c.Add(KCmdSetOtableBase64, b, 24);
		}
	{ const TUint32 b[5] = { 1, KMobFmtRange, (phys + KOffImage) >> 12, 0, KW * KH * 4 }; c.Add(KCmdDefineGbMob64, b, 20); }
	{ const TUint32 b[9] = { 1, KSurfScreenTarget | KSurfBindRenderTarget | KSurfBindShaderResource, KFmtB8G8R8A8Unorm, 1, 0, 0, KW, KH, 1 };
	  c.Add(KCmdDefineGbSurface, b, 36); }
	{ const TUint32 b[2] = { 1, 1 }; c.Add(KCmdBindGbSurface, b, 8); }
	TUint32* px = (TUint32*)(base + KOffImage);
	for (TInt y = 0; y < KH; ++y)
		for (TInt x = 0; x < KW; ++x)
			{
			TUint32 v = (x < KW / 3) ? 0xFF0000u : (x < 2 * KW / 3) ? 0x00C000u : 0x0000FFu;
			if (x < 6 || y < 6 || x >= KW - 6 || y >= KH - 6)
				v = 0xFFFFFFu;
			px[y * KW + x] = v | 0xFF000000u;
			}
	{ const TUint32 b[9] = { 1, 0, 0, 0, 0, 0, KW, KH, 1 }; c.Add(KCmdUpdateGbImage, b, 36); }
	{ const TUint32 b[7] = { 0, KW, KH, 0, 0, KStFlagPrimary, 0 }; c.Add(KCmdDefineGbScreenTarget, b, 28); }
	{ const TUint32 b[4] = { 0, 1, 0, 0 }; c.Add(KCmdBindGbScreenTarget, b, 16); }
	{ const TUint32 b[5] = { 0, 0, 0, KW, KH }; c.Add(KCmdUpdateGbScreenTarget, b, 20); }
	if (Run(ch, c, 0, 0, "S2 tables+MOB+surface+screen target (CPU ramp)") != KErrNone)
		return KErrGeneral;
	RDebug::Printf("PH: svga S2: screen target shown");
	}
	User::After(9 * 1000000);
	// ---- 2: the DX context ----
	{
	TCmds c(base, KOffCmds);
	{ const TUint32 b[5] = { 2, KMobFmtRange, (phys + KOffDxCtx) >> 12, 0, KDxCtxBytes }; c.Add(KCmdDefineGbMob64, b, 20); }
	for (TInt i = 0; i < KNumCoTables; ++i)
		{ const TUint32 b[5] = { (TUint32)(3 + i), KMobFmtRange, (phys + KOffCoTables + i * 0x1000) >> 12, 0, 0x1000 }; c.Add(KCmdDefineGbMob64, b, 20); }
	{ const TUint32 b[1] = { 0 }; c.Add(KCmdDxDefineContext, b, 4); }
	{ const TUint32 b[3] = { 0, 2, 0 }; c.Add(KCmdDxBindContext, b, 12); }
	for (TInt i = 0; i < KNumCoTables; ++i)
		{ const TUint32 b[4] = { 0, (TUint32)(3 + i), (TUint32)i, 0 }; c.Add(KCmdDxSetCoTable, b, 16); }
	if (Run(ch, c, 0, 0, "S2b DX context + COTables") != KErrNone)
		return KErrGeneral;
	}
	// ---- 3: render target view + the GPU clear (orange) + present ----
	{
	TCmds c(base, KOffCmds);
	{ const TUint32 b[7] = { 0, 1, KFmtB8G8R8A8Unorm, KResTexture2D, 0, 0, 1 }; c.Add(KCmdDxDefineRenderTargetView, b, 28); }
	{ const TUint32 b[5] = { 0, 0x3F800000u, 0x3F000000u, 0, 0x3F800000u }; c.Add(KCmdDxClearRenderTargetView, b, 20); }
	if (Run(ch, c, KCbFlagDx, 0, "S2b DX RTV + clear orange") != KErrNone)
		return KErrGeneral;
	TCmds u(base, KOffCmds);
	{ const TUint32 b[5] = { 0, 0, 0, KW, KH }; u.Add(KCmdUpdateGbScreenTarget, b, 20); }
	if (Run(ch, u, 0, 0, "S2b present") != KErrNone)
		return KErrGeneral;
	RDebug::Printf("PH: svga S2b: GPU clear done + screen target updated");
	}
	User::After(25 * 1000000);
	{
	TCmds c(base, KOffCmds);
	{ const TUint32 b[1] = { 0 }; c.Add(KCmdDestroyGbScreenTarget, b, 4); }
	Run(ch, c, 0, 0, "destroy screen target");
	}
	arena.Close();
	ch.Close();
	RDebug::Printf("PH: svga done PASS");
	return KErrNone;
	}

TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TInt r = MainL();
	delete cleanup;
	__UHEAP_MARKEND;
	return r;
	}
