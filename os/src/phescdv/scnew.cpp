// Copyright The Phorepoc Authors
// SPDX-License-Identifier: Apache-2.0

// scnew.cpp -- OURS: the screen driver's factory for this board (replaces the OMAP driver's smomap/scnew.cpp in the ROM's scdv.dll).
//
// Why: the board needs a draw device that rotates in software (an application that asks for landscape: os/board/wsini.txt), and only
// the OMAP driver's draw devices offer the rotated orientations. Their ConstructScreenL reads the display's row pitch from the HAL
// (EDisplayOffsetBetweenLines) and then calls the base class's Construct(aSize), which sets the row pitch again to exactly
// width * 4 bytes: the pitch the HAL reports is lost. The display here lays the screen out with the pitch of the real
// phones (rows padded to 64 bytes: 1472 bytes for 360 pixels; applications that write the screen memory directly assume it), so the
// window server drew with a 1440 byte pitch into a 1472 byte one and the whole screen came out sheared (seen in VirtualBox, whose VMSVGA
// path uses the padded shadow screen; QEMU draws straight into a framebuffer of exactly 1440 bytes per row, where the two agree).
//
// What: the three 32-bit classes are constructed through a subclass that puts the HAL's pitch back after the base class has run
// (iScanLineWords and iLongWidth are the device's stride, in words and in pixels; SetSize keeps them; the pixel routines, rotated or
// not, address rows with iScanLineWords). The factory below does the same as the OMAP one otherwise: the draw device for the display
// mode that matches the HAL's bits per pixel, over the HAL's screen address and size. The 8- and 16-bit classes are the OMAP
// driver's own (this display is 32 bits).
#include <bitdraw.h>
#include "smomap/scdraw.h"
#include <hal.h>
#include "ScreenInfo.h"
#include <graphics/gdi/gdiconsts.h>

// A screen draw device whose stride is the HAL's row pitch.
template <class T>
class TPhePitchedScreen : public T
	{
public:
	virtual TInt ConstructScreenL(TInt aScreenNo, TAny* aBitmapAddress, TSize aSize)
		{
		const TInt r = T::ConstructScreenL(aScreenNo, aBitmapAddress, aSize);
		if (r != KErrNone)
			return r;
		TInt pitch = 0;
		TInt err = HAL::Get(aScreenNo, HALData::EDisplayMode, pitch);		// (the attribute takes the display mode as its in-value)
		if (err == KErrNone)
			err = HAL::Get(aScreenNo, HALData::EDisplayOffsetBetweenLines, pitch);
		if (err != KErrNone)
			return err;
		if ((pitch & 3) || (pitch >> 2) < aSize.iWidth)
			return KErrArgument;
		this->iScanLineWords = pitch >> 2;
		this->iLongWidth = pitch >> 2;
		return KErrNone;
		}
	};

template <class T>
static CFbsDrawDevice* NewScreenL(TInt aScreenNo, const TScreenInfo& aInfo)
	{
	T* device = new (ELeave) T;
	CleanupStack::PushL(device);
	User::LeaveIfError(device->ConstructScreenL(aScreenNo, aInfo.iAddress, aInfo.iSize));
	CleanupStack::Pop(device);
	return device;
	}

static CFbsDrawDevice* CreateInstanceL(TInt aScreenNo, TDisplayMode aDispMode, const TScreenInfo& aInfo)
	{
	TInt bpp = 0;
	if (HAL::Get(aScreenNo, HALData::EDisplayMode, bpp) != KErrNone || HAL::Get(aScreenNo, HALData::EDisplayBitsPerPixel, bpp) != KErrNone)
		User::Leave(KErrNotSupported);
	const TBool deep = (bpp == 24 || bpp == 32) && aDispMode == CFbsDrawDevice::DisplayMode16M();		// 24 and 32 bits are both 32 bits of memory per pixel
	switch (aDispMode)
		{
	case EColor256:
		if (bpp != 8)
			User::Leave(KErrNotSupported);
		return NewScreenL<CDrawEightBppScreenBitmapColor>(aScreenNo, aInfo);
	case EColor64K:
		if (bpp != 16)
			User::Leave(KErrNotSupported);
		return NewScreenL<CDrawSixteenBppScreenBitmap>(aScreenNo, aInfo);
	case EColor16MU:
		if (!deep)
			User::Leave(KErrNotSupported);
		return NewScreenL<TPhePitchedScreen<CDrawUTwentyFourBppScreenBitmap> >(aScreenNo, aInfo);
	case EColor16MA:
		if (!deep)
			User::Leave(KErrNotSupported);
		return NewScreenL<TPhePitchedScreen<CDrawThirtyTwoBppScreenBitmapAlpha> >(aScreenNo, aInfo);
	case EColor16MAP:
		if (!deep)
			User::Leave(KErrNotSupported);
		return NewScreenL<TPhePitchedScreen<CDrawThirtyTwoBppScreenBitmapAlphaPM> >(aScreenNo, aInfo);
	default:
		User::Leave(KErrNotSupported);
		}
	return NULL;
	}

EXPORT_C CFbsDrawDevice* CFbsDrawDevice::NewScreenDeviceL(TScreenInfoV01 aInfo, TDisplayMode aDispMode)
	{
	__ASSERT_ALWAYS(aInfo.iScreenAddressValid, Panic(EScreenDriverPanicInvalidWindowHandle));
	const TScreenInfo info(aInfo.iScreenAddress, aInfo.iScreenSize);
	return CreateInstanceL(KDefaultScreenNo, aDispMode, info);
	}

EXPORT_C CFbsDrawDevice* CFbsDrawDevice::NewScreenDeviceL(TInt aScreenNo, TDisplayMode aDispMode)
	{
	TInt address = 0, width = 0, height = 0;
	User::LeaveIfError(HAL::Get(aScreenNo, HALData::EDisplayMemoryAddress, address));
	User::LeaveIfError(HAL::Get(aScreenNo, HALData::EDisplayXPixels, width));
	User::LeaveIfError(HAL::Get(aScreenNo, HALData::EDisplayYPixels, height));
	__ASSERT_ALWAYS(width > 0 && height > 0 && address != 0, Panic(EScreenDriverPanicInvalidHalValue));
	const TScreenInfo info(reinterpret_cast<TAny*>(address), TSize(width, height));
	return CreateInstanceL(aScreenNo, aDispMode, info);
	}

// the 16M mode this driver offers: 32 bits per pixel with alpha
EXPORT_C TDisplayMode CFbsDrawDevice::DisplayMode16M()
	{
	return EColor16MA;
	}
