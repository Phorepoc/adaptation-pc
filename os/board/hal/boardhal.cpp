// Copyright The Phorepoc Authors
// SPDX-License-Identifier: Apache-2.0

// boardhal.cpp -- the Phorepoc amd64 board's own HAL attribute handler (the rest are the generic userhal.cpp ones).
//
// EDisplayNumberOfScreens is a constant on a phone (values.hda); here the display driver may add virtual screens
// (xk_display.cpp, make_ekern.sh VSCREENS=n), so the count comes from the kernel's
// variant through the documented EVariantHalCurrentNumberOfScreens (u32hal.h). WSERV sizes its screen array from it
// (nonnga/SERVER/WSTOP.CPP:418). A kernel without that variant function = one screen.
#include <kernel/hal_int.h>
#include "u32std.h"

TInt GetDisplayNumberOfScreens(TInt /*aDeviceNumber*/, TInt /*aAttrib*/, TBool /*aSet*/, TAny* aInOut)
	{
	TInt n = 1;
	if (UserSvr::HalFunction(EHalGroupVariant, EVariantHalCurrentNumberOfScreens, &n, 0) != KErrNone || n < 1)
		n = 1;
	*(TInt*)aInOut = n;
	return KErrNone;
	}
