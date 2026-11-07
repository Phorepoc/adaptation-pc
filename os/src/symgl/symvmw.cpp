// Copyright The Phorepoc Authors
// SPDX-License-Identifier: Apache-2.0

// symvmw.cpp -- OURS (S3): symvmw.h (core's seam header) over the SVGA3D kernel driver's channel (RVmwSvgaChannel,
// adaptation-x86-64: tools/phorepoc-x86/ekern). Part of phgl.dll on this board (the registry's EXTEND adds it to core's phgl):
// Mesa's svga driver reaches the VirtualBox / VMware host GPU through it.
#include <e32debug.h>
#include <e32std.h>
#include <e32hal.h>
#include "symvmw.h"
#include "rvmwsvgachannel.h"

struct symvmw
	{
	RVmwSvgaChannel iCh;
	RChunk iArena, iPool;
	TVmwSvgaInfo iInfo;
	};

symvmw_t* symvmw_open(void)
	{
	symvmw* v = new symvmw;
	if (!v)
		return NULL;
	const char* step = "channel open";
	TInt r = v->iCh.Open();
	if (r == KErrNone)
		{
		step = "info";
		r = v->iCh.GetInfo(v->iInfo);
		}
	if (r == KErrNone && (!v->iInfo.iDevCaps[0] || !v->iInfo.iDevCaps[1] || !(v->iInfo.iCaps & 0x8000000u)))
		{
		step = "caps (3D, DX contexts, guest-backed objects)";
		r = KErrNotSupported;		// 3D off, no DX contexts, or no guest-backed objects: Mesa's svga driver cannot run
		}
	if (r == KErrNone)
		{
		step = "arena";
		r = v->iCh.OpenArena(v->iArena);
		}
	if (r == KErrNone)
		{
		step = "pool";
		r = v->iCh.OpenPool(v->iPool);
		}
	if (r != KErrNone)
		{
		RDebug::Printf("[symgl] symvmw_open: %s -> %d (the software renderer instead)", step, r);
		v->iPool.Close();
		v->iArena.Close();
		v->iCh.Close();
		delete v;
		return NULL;
		}
	return v;
	}

void symvmw_close(symvmw_t* v)
	{
	if (!v)
		return;
	v->iPool.Close();
	v->iArena.Close();
	v->iCh.Close();
	delete v;
	}

void symvmw_info(symvmw_t* v, symvmw_info_t* o)
	{
	Mem::FillZ(o, sizeof(*o));
	o->caps = v->iInfo.iCaps;
	o->cap2 = v->iInfo.iCap2;
	o->devcap_3d = v->iInfo.iDevCaps[0];
	o->devcap_dx = v->iInfo.iDevCaps[1];
	o->devcap_sm41 = v->iInfo.iDevCaps[2];
	o->devcap_sm5 = v->iInfo.iDevCaps[3];
	o->devcap_gl43 = v->iInfo.iDevCaps[4];
	o->max_texture = v->iInfo.iDevCaps[5];
	o->mob_max_kb = v->iInfo.iMobMaxKb;
	o->display_w = v->iInfo.iDisplayW;
	o->display_h = v->iInfo.iDisplayH;
	// the guest memory a process may pin for the device: its pool's address space, but no more than half of the RAM that is free now
	TMemoryInfoV1Buf mem;
	TUint32 limit = v->iInfo.iPoolBytes;
	if (UserHal::MemoryInfo(mem) == KErrNone)
		{
		const TUint32 half = (TUint32)(mem().iFreeRamInBytes / 2);
		limit = Min(limit, Max(half, (TUint32)(16 << 20)));
		}
	o->mob_memory = limit;
	o->pool_base = v->iPool.Base();
	o->pool_bytes = v->iInfo.iPoolBytes;
	o->arena_base = v->iArena.Base();
	o->arena_bytes = v->iInfo.iArenaSize;
	o->mob_base = v->iInfo.iMobBase; o->mob_count = v->iInfo.iMobCount;
	o->surf_base = v->iInfo.iSurfBase; o->surf_count = v->iInfo.iSurfCount;
	o->ctx_base = v->iInfo.iCtxBase; o->ctx_count = v->iInfo.iCtxCount;
	}

int symvmw_devcaps(symvmw_t* v, uint32_t* caps, unsigned count)
	{
	TVmwDevCaps* c = new TVmwDevCaps;
	if (!c)
		return KErrNoMemory;
	TInt r = v->iCh.GetDevCaps(*c);
	if (r == KErrNone)
		for (unsigned i = 0; i < count; i++)
			caps[i] = i < (unsigned)KVmwNumDevCaps ? c->iCaps[i] : 0;
	delete c;
	return r;
	}

int symvmw_pool_commit(symvmw_t* v, uint32_t offset, uint32_t size)
	{
	return v->iCh.PoolCommit(offset, size);
	}

int symvmw_mob_define(symvmw_t* v, uint32_t mobid, uint32_t offset, uint32_t size)
	{
	return v->iCh.DefineMob(mobid, offset, size);
	}

int symvmw_mob_destroy(symvmw_t* v, uint32_t mobid)
	{
	return v->iCh.DestroyMob(mobid);
	}

int symvmw_scanout(symvmw_t* v, int op)
	{
	return v->iCh.Scanout(op);
	}

int symvmw_submit(symvmw_t* v, uint32_t offset, uint32_t length, uint32_t context, uint32_t flags, uint32_t dx_context,
                  uint32_t surf_high, uint32_t ctx_high, uint32_t* status, uint32_t* error_offset)
	{
	TVmwSubmit s;
	Mem::FillZ(&s, sizeof(s));
	s.iOffset = offset;
	s.iLength = length;
	s.iContext = context;
	s.iFlags = flags;
	s.iDxContext = dx_context;
	s.iSurfHigh = surf_high;
	s.iCtxHigh = ctx_high;
	const TInt r = v->iCh.Submit(s);
	*status = s.iStatus;
	*error_offset = s.iErrorOffset;
	return r;
	}
