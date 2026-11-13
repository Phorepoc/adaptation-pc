// Copyright The Phorepoc Authors
// SPDX-License-Identifier: Apache-2.0

// openwfc_ri_displayhal.cpp -- the PC board's display back end for OpenWF-C (libWFC's COpenWFC_RI_Display), in place of
// core's display-channel one (os/src/nga/openwfc_ri_display): this board has no display channel driver. Its display is the
// shadow screen of the kernel's display HAL (adaptation-x86-64 xk_display.cpp), which a flusher copies to the real framebuffer
// every 20 ms, diffing as it goes. So the back end only has to put each composed frame into that memory: it gets the screen as
// an RChunk (HAL EDisplayMemoryHandle) and copies the scene there row by row, at the display's row pitch (rows are padded to
// 64 bytes). A rotated screen mode (the landscape mode of this board, rotated 90) is written turned into the portrait shadow, and
// the display is told its orientation (HAL function 0x7103, as the board's classic screen driver does), so the flusher shows it
// upright in a landscape window.
// Same DLL, UIDs and exports as core's (registry/components_pc.py replaces only its source).
#include "openwfc_ri_displayhal.h"
#include <e32debug.h>
#include <e32svr.h>
#include <hal.h>
#include <u32hal.h>

const TUint32 KDefaultScreenNumber = 0;
const TInt KHalPhoreRotate = 0x7103;			// xk_display.cpp EDisplayHalPhoreRotate: 0 normal, 1 rotated 90, 3 rotated 270
// RDisplayChannel::TDisplayRotation bits (what libWFC's display policy expects in EScreenAttributeSupportedRotation)
const TUint32 KRotAll = 0xF;					// all four: libWFC then composes unrotated and hands the rotation here (owfscreen.cpp)

EXPORT_C COpenWFC_RI_Display* COpenWFC_RI_Display::NewL(TUint aScreen)
	{
	return COpenWFC_RI_DisplayHal::NewL(aScreen);
	}

EXPORT_C TUint32 COpenWFC_RI_Display::DefaultScreenNumber()
	{
	return KDefaultScreenNumber;
	}

TInt COpenWFC_RI_Display::GetAttributeSize(TUint aAttributeId)
	{
	switch (aAttributeId)
		{
	case EScreenAttributeImplementationVersion:
	case EScreenAttributePixelFormat:
	case EScreenAttributeBytesPerPixel:
	case EScreenAttributeSupportedRotation:
	case EScreenAttributeNormalWidth:
	case EScreenAttributeNormalHeight:
	case EScreenAttributeNormalStride:
	case EScreenAttributeFlippedWidth:
	case EScreenAttributeFlippedHeight:
	case EScreenAttributeFlippedStride:
	case EScreenAttributeDefaultRotation:
	case EScreenAttributeCurrentRotation:
		return sizeof(TUint32);
	case EScreenAttributeScreenGeometry:
		return sizeof(TScreenGeometryAttribute);
	default:
		return 0;
		}
	}

COpenWFC_RI_DisplayHal::COpenWFC_RI_DisplayHal(TUint aScreen)
	: iScreenNumber(aScreen), iCurrentSceneStream(SYMBIAN_INVALID_HANDLE)
	{
	}

COpenWFC_RI_DisplayHal* COpenWFC_RI_DisplayHal::NewL(TUint aScreen)
	{
	COpenWFC_RI_DisplayHal* self = new(ELeave) COpenWFC_RI_DisplayHal(aScreen);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

COpenWFC_RI_DisplayHal::~COpenWFC_RI_DisplayHal()
	{
	if (iShownRotation != EScreenRotate0)
		UserSvr::HalFunction(EHalGroupDisplay, KHalPhoreRotate, (TAny*)0, NULL);
	iScreen.Close();
	}

void COpenWFC_RI_DisplayHal::ConstructL()
	{
	TInt h = 0;
	User::LeaveIfError(HAL::Get(iScreenNumber, HALData::EDisplayMemoryHandle, h));
	User::LeaveIfError(iScreen.SetReturnedHandle(h));
	TInt w = 0, ht = 0, mode = 0, pitch = 0;
	User::LeaveIfError(HAL::Get(iScreenNumber, HALData::EDisplayXPixels, w));
	User::LeaveIfError(HAL::Get(iScreenNumber, HALData::EDisplayYPixels, ht));
	User::LeaveIfError(HAL::Get(iScreenNumber, HALData::EDisplayMode, mode));
	pitch = mode;
	User::LeaveIfError(HAL::Get(iScreenNumber, HALData::EDisplayOffsetBetweenLines, pitch));
	if (w <= 0 || ht <= 0 || pitch < w * 4 || pitch * ht > iScreen.Size())
		{
		RDebug::Printf("PH: [wfchal] unusable screen %dx%d pitch %d chunk %d", w, ht, pitch, iScreen.Size());
		User::Leave(KErrNotSupported);
		}
	iWidth = w;
	iHeight = ht;
	iPitch = pitch;
	iScreenInfo.iPixelFormat = EUidPixelFormatXRGB_8888;
	iScreenInfo.iBytesPerPixel = 4;
	iScreenInfo.iSupportedRotations = KRotAll;
	iScreenInfo.iNormalWidth = w;
	iScreenInfo.iNormalHeight = ht;
	iScreenInfo.iNormalStride = w * 4;			// what the scene is composed at (the copy pads the rows)
	iScreenInfo.iFlippedWidth = ht;
	iScreenInfo.iFlippedHeight = w;
	iScreenInfo.iFlippedStride = ht * 4;
	iScreenInfo.iDefaultRotation = EScreenRotate0;
	iScreenInfo.iCurrentRotation = EScreenRotate0;
	iNewRotation = EScreenRotate0;
	iShownRotation = EScreenRotate0;
	RDebug::Printf("PH: [wfchal] display %dx%d pitch %d, chunk %d bytes", w, ht, pitch, iScreen.Size());
	}

TInt COpenWFC_RI_DisplayHal::GetAttribute(TInt aAttributeId, TAny* aAttribute, TInt aAttributeSize)
	{
	const TInt size = GetAttributeSize(aAttributeId);
	if (!aAttribute)
		return KErrArgument;
	if (!size)
		return KErrNotSupported;
	if (aAttributeSize != size)
		return KErrArgument;
	TUint32* v = (TUint32*)aAttribute;
	switch (aAttributeId)
		{
	case EScreenAttributeImplementationVersion:	*v = KImplementationVersion; break;
	case EScreenAttributePixelFormat:			*v = iScreenInfo.iPixelFormat; break;
	case EScreenAttributeBytesPerPixel:			*v = iScreenInfo.iBytesPerPixel; break;
	case EScreenAttributeSupportedRotation:		*v = iScreenInfo.iSupportedRotations; break;
	case EScreenAttributeNormalWidth:			*v = iScreenInfo.iNormalWidth; break;
	case EScreenAttributeNormalHeight:			*v = iScreenInfo.iNormalHeight; break;
	case EScreenAttributeNormalStride:			*v = iScreenInfo.iNormalStride; break;
	case EScreenAttributeFlippedWidth:			*v = iScreenInfo.iFlippedWidth; break;
	case EScreenAttributeFlippedHeight:			*v = iScreenInfo.iFlippedHeight; break;
	case EScreenAttributeFlippedStride:			*v = iScreenInfo.iFlippedStride; break;
	case EScreenAttributeDefaultRotation:		*v = iScreenInfo.iDefaultRotation; break;
	case EScreenAttributeCurrentRotation:		*v = iScreenInfo.iCurrentRotation; break;
	case EScreenAttributeScreenGeometry:		*(TScreenGeometryAttribute*)aAttribute = iScreenInfo; break;
	default:									return KErrNotSupported;
		}
	return KErrNone;
	}

TInt COpenWFC_RI_DisplayHal::SetAttribute(TInt aAttributeId, TAny* aAttribute, TInt aAttributeSize)
	{
	if (aAttributeSize != GetAttributeSize(aAttributeId) || !aAttribute)
		return KErrArgument;
	if (aAttributeId != EScreenAttributeCurrentRotation)
		return KErrNotSupported;
	const TScreenRotation r = static_cast<TScreenRotation>(*(TUint32*)aAttribute);
	iNewRotation = r;
	return KErrNone;
	}

TInt COpenWFC_RI_DisplayHal::CommitAttribute()
	{
	iScreenInfo.iCurrentRotation = iNewRotation;	// applied with the next frame (UpdateDisplay)
	return KErrNone;
	}

TInt COpenWFC_RI_DisplayHal::SetLayerSurface(TInt aLayer, SymbianStreamType aStream, TInt* aNonTrivialAttribs)
	{
	if (aLayer != 0)
		return KErrArgument;
	if (aNonTrivialAttribs && *aNonTrivialAttribs)
		return KErrNotSupported;
	khronos_int32_t format = 0, pixSize = 0;
	SymbianStreamGetHeader(aStream, NULL, NULL, NULL, &format, &pixSize);
	if (format != EUidPixelFormatARGB_8888 && format != EUidPixelFormatARGB_8888_PRE && format != EUidPixelFormatXRGB_8888)
		return KErrNotSupported;
	iCurrentSceneStream = aStream;
	return KErrNone;
	}

TInt COpenWFC_RI_DisplayHal::SetTopLayerSurface(SymbianStreamType aStream, TInt* aNonTrivialAttribs)
	{
	return SetLayerSurface(KTopMostLayer, aStream, aNonTrivialAttribs);
	}

TInt COpenWFC_RI_DisplayHal::UpdateDisplay()
	{
	if (iCurrentSceneStream == SYMBIAN_INVALID_HANDLE)
		return KErrNotReady;
	const TScreenRotation rot = iScreenInfo.iCurrentRotation;
	const TBool turned = rot == EScreenRotate90 || rot == EScreenRotate270;
	khronos_int32_t sw = 0, sh = 0, sStride = 0, sFmt = 0, sPix = 0;
	SymbianStreamGetHeader(iCurrentSceneStream, &sw, &sh, &sStride, &sFmt, &sPix);
	const TInt dw = turned ? iHeight : iWidth, dh = turned ? iWidth : iHeight;		// the scene as composed
	const TInt w = Min((TInt)sw, dw), h = Min((TInt)sh, dh);
	const TInt srcStride = sStride >= w * 4 ? (TInt)sStride : w * 4;
	SymbianStreamBuffer front;
	if (SymbianStreamAcquireReadBuffer(iCurrentSceneStream, &front) != KErrNone)
		return KErrGeneral;
	void* src = NULL;
	SymbianStreamGetBufferPointer(iCurrentSceneStream, front, &src);
	TUint8* dst = iScreen.Base();
	const TInt wpl = iPitch / 4;
	if (src)
		{
		if (rot == EScreenRotate180)
			{
			// the display cannot turn upside down: the frame is written turned half way round
			TUint32* d = (TUint32*)dst;
			for (TInt y = 0; y < h; ++y)
				{
				const TUint32* s = (const TUint32*)((const TUint8*)src + y * srcStride);
				TUint32* row = d + (iHeight - 1 - y) * wpl + iWidth - 1;
				for (TInt x = 0; x < w; ++x)
					*row-- = s[x];
				}
			}
		else if (!turned)
			{
			for (TInt y = 0; y < h; ++y)
				Mem::Copy(dst + y * iPitch, (const TUint8*)src + y * srcStride, w * 4);
			}
		else
			{
			// the flusher shows shadow (W-1-dy, dx) at display (dx,dy) for orientation 1 and (dy, H-1-dx) for 3 (xk_display.cpp ScreenPixelAt)
			TUint32* d = (TUint32*)dst;
			for (TInt y = 0; y < h; ++y)
				{
				const TUint32* s = (const TUint32*)((const TUint8*)src + y * srcStride);
				if (rot == EScreenRotate90)
					for (TInt x = 0; x < w; ++x)
						d[x * wpl + (iWidth - 1 - y)] = s[x];
				else
					for (TInt x = 0; x < w; ++x)
						d[(iHeight - 1 - x) * wpl + y] = s[x];
				}
			}
		}
	SymbianStreamReleaseReadBuffer(iCurrentSceneStream, front);
	if (rot != iShownRotation)
		{
		UserSvr::HalFunction(EHalGroupDisplay, KHalPhoreRotate, (TAny*)(rot == EScreenRotate90 ? 1 : rot == EScreenRotate270 ? 3 : 0), NULL);
		iShownRotation = rot;
		RDebug::Printf("PH: [wfchal] rotation %d (scene %dx%d stride %d)", (TInt)rot, (TInt)sw, (TInt)sh, (TInt)sStride);
		}
	if (++iFrames == 1 || iFrames % 500 == 0)
		RDebug::Printf("PH: [wfchal] frame %u: scene %dx%d stride %d", iFrames, (TInt)sw, (TInt)sh, (TInt)sStride);
	return KErrNone;
	}
