// Copyright The Phorepoc Authors
// SPDX-License-Identifier: Apache-2.0

// openwfc_ri_displayhal.h -- the PC board's OpenWF-C display back end over the display HAL's screen memory (see the .cpp)
#ifndef OPENWFC_RI_DISPLAYHAL_H
#define OPENWFC_RI_DISPLAYHAL_H

#include <WF/openwfc_ri_display.h>

NONSHARABLE_CLASS(COpenWFC_RI_DisplayHal) : public COpenWFC_RI_Display
	{
public:
	~COpenWFC_RI_DisplayHal();
	static COpenWFC_RI_DisplayHal* NewL(TUint aScreen);
	TInt GetAttribute(TInt aAttributeId, TAny* aAttribute, TInt aAttributeSize);
	TInt SetAttribute(TInt aAttributeId, TAny* aAttribute, TInt aAttributeSize);
	TInt CommitAttribute();
	TInt SetLayerSurface(TInt aLayer, SymbianStreamType aStream, TInt* aNonTrivialAttribs);
	TInt UpdateDisplay();
	TInt SetTopLayerSurface(SymbianStreamType aStream, TInt* aNonTrivialAttribs);
private:
	COpenWFC_RI_DisplayHal(TUint aScreen);
	void ConstructL();
private:
	TUint iScreenNumber;
	RChunk iScreen;						// the shadow screen (HAL EDisplayMemoryHandle)
	TInt iWidth, iHeight, iPitch;		// its portrait size and row pitch in bytes
	TScreenGeometryAttribute iScreenInfo;
	TScreenRotation iNewRotation;
	TScreenRotation iShownRotation;		// the orientation the display was last told
	SymbianStreamType iCurrentSceneStream;
	TUint32 iFrames;
public:
	static const TInt32 KImplementationVersion = 0x00000001;
	};

#endif
