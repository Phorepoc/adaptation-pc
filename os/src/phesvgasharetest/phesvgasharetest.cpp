// Copyright The Phorepoc Authors
// SPDX-License-Identifier: Apache-2.0

// phesvgasharetest.cpp -- self test of the SVGA channel's shared-surface table (xk_vmwsvga.cpp EVmwCtrlExportSurface / ImportSurface,
// hildon-port.md 7.43): channel A exports one of its surface ids with a description, channel B imports it by token and gets the
// same; B may not export A's ids; after A closes, the token no longer imports. Prints "[phesvgasharetest] PASS" or the first failure.
#include <e32std.h>
#include <e32debug.h>
#include "rvmwsvgachannel.h"

#define CHECK(c) do { if (!(c)) { RDebug::Printf("[phesvgasharetest] FAIL line %d: %s", __LINE__, #c); return KErrGeneral; } } while (0)

static TInt Run()
	{
	RVmwSvgaChannel a, b;
	TInt r = a.Open();
	if (r != KErrNone)
		{
		RDebug::Printf("[phesvgasharetest] no SVGA channel (%d): not a VMSVGA 3D machine", r);
		return r;
		}
	CHECK(b.Open() == KErrNone);
	TVmwSvgaInfo ia, ib;
	CHECK(a.GetInfo(ia) == KErrNone && b.GetInfo(ib) == KErrNone && ia.iSurfBase != ib.iSurfBase);

	TVmwShare sh = { 0, ia.iSurfBase + 5, 640, 360, 2 };
	CHECK(a.ExportSurface(sh) == KErrNone && sh.iToken > 0);
	const TInt token = sh.iToken;

	TVmwShare in = { token, 0, 0, 0, 0 };
	CHECK(b.ImportSurface(in) == KErrNone);
	CHECK(in.iSid == ia.iSurfBase + 5 && in.iWidth == 640 && in.iHeight == 360 && in.iFormat == 2);

	TVmwShare foreign = { 0, ia.iSurfBase + 6, 16, 16, 2 };
	CHECK(b.ExportSurface(foreign) == KErrArgument);				// only the caller's own surfaces
	CHECK(b.UnexportSurface(token) == KErrNotFound);				// only the owner unexports

	a.Close();														// the owner goes: its surfaces, and their shares
	TVmwShare gone = { token, 0, 0, 0, 0 };
	CHECK(b.ImportSurface(gone) == KErrNotFound);
	b.Close();
	RDebug::Printf("[phesvgasharetest] PASS: export, import by token, foreign export refused, share ends with the owner");
	return KErrNone;
	}

GLDEF_C TInt E32Main()
	{
	return Run();
	}
