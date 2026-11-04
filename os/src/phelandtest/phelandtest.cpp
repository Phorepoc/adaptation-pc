// Copyright The Phorepoc Authors
// SPDX-License-Identifier: Apache-2.0

// phelandtest.cpp -- OURS: what an application that asks for landscape (SetOrientationL) makes the window server do, without Avkon: switch the
// screen to its landscape mode and draw a test pattern with the window gc. The pattern has a mark at every corner (white top left, red top
// right, green bottom right, blue bottom left), a one-pixel frame and a diagonal, so a wrong pitch or a wrong rotation is visible at a glance.
// Prints "PH: land" lines: the screen's size and mode, and the pixel the gc read back is not needed (the screenshot is the verdict).
#include <e32std.h>
#include <e32base.h>
#include <e32debug.h>
#include <w32std.h>

static void Pattern(CWindowGc& aGc, const TSize& aSize)
	{
	aGc.SetPenStyle(CGraphicsContext::ENullPen);
	aGc.SetBrushStyle(CGraphicsContext::ESolidBrush);
	aGc.SetBrushColor(TRgb(0x20, 0x20, 0x30));
	aGc.DrawRect(TRect(TPoint(0, 0), aSize));
	for (TInt x = 0; x < aSize.iWidth; x += 40)				// vertical bars of a ramp: a stride error shears these
		{
		aGc.SetBrushColor(TRgb(x * 255 / aSize.iWidth, 128, 255 - x * 255 / aSize.iWidth));
		aGc.DrawRect(TRect(x, aSize.iHeight / 3, Min(x + 20, aSize.iWidth), aSize.iHeight * 2 / 3));
		}
	const TInt m = 40;
	aGc.SetBrushColor(KRgbWhite);   aGc.DrawRect(TRect(0, 0, m, m));
	aGc.SetBrushColor(KRgbRed);     aGc.DrawRect(TRect(aSize.iWidth - m, 0, aSize.iWidth, m));
	aGc.SetBrushColor(KRgbGreen);   aGc.DrawRect(TRect(aSize.iWidth - m, aSize.iHeight - m, aSize.iWidth, aSize.iHeight));
	aGc.SetBrushColor(KRgbBlue);    aGc.DrawRect(TRect(0, aSize.iHeight - m, m, aSize.iHeight));
	aGc.SetPenStyle(CGraphicsContext::ESolidPen);
	aGc.SetPenColor(KRgbYellow);
	aGc.DrawLine(TPoint(0, 0), TPoint(aSize.iWidth - 1, aSize.iHeight - 1));
	aGc.SetBrushStyle(CGraphicsContext::ENullBrush);
	aGc.DrawRect(TRect(TPoint(0, 0), aSize));
	}

static void MainL()
	{
	RWsSession ws;
	User::LeaveIfError(ws.Connect());
	CleanupClosePushL(ws);
	CWsScreenDevice* screen = new (ELeave) CWsScreenDevice(ws);
	CleanupStack::PushL(screen);
	User::LeaveIfError(screen->Construct());
	RWindowGroup group(ws);
	User::LeaveIfError(group.Construct(0x4c414e44, ETrue));
	CleanupClosePushL(group);
	RWindow win(ws);
	User::LeaveIfError(win.Construct(group, 0x4c414e45));
	CleanupClosePushL(win);
	CWindowGc* gc;
	User::LeaveIfError(screen->CreateContext(gc));
	CleanupStack::PushL(gc);
	group.SetOrdinalPosition(0, 1000);
	win.Activate();
	const TInt modes = screen->NumScreenModes();
	TInt want = 0;
	TSize wantSize(0, 0);
	for (TInt i = 0; i < modes; ++i)						// the landscape mode: wider than high
		{
		TPixelsTwipsAndRotation p;
		screen->GetScreenModeSizeAndRotation(i, p);
		RDebug::Printf("PH: land mode %d = %dx%d rotation %d", i, p.iPixelSize.iWidth, p.iPixelSize.iHeight, (TInt)p.iRotation);
		if (p.iPixelSize.iWidth > p.iPixelSize.iHeight)
			{
			want = i;
			wantSize = p.iPixelSize;
			}
		}
	TSize last(0, 0);
	for (TInt n = 0; ; ++n)
		{
		if (screen->CurrentScreenMode() != want)			// (Avkon's cap server puts the home screen's mode back when it takes the focus)
			screen->SetScreenMode(want);
		const TSize size = wantSize;							// (the mode's own size: SizeInPixels() is the window server's view of the current mode)
		if (size != last || n % 20 == 0)
			{
			RDebug::Printf("PH: land screen mode %d mode size %dx%d SizeInPixels %dx%d", screen->CurrentScreenMode(), size.iWidth, size.iHeight,
						screen->SizeInPixels().iWidth, screen->SizeInPixels().iHeight);
			last = size;
			win.SetExtent(TPoint(0, 0), size);
			win.Invalidate();
			}
		win.BeginRedraw();
		gc->Activate(win);
		Pattern(*gc, size);
		gc->Deactivate();
		win.EndRedraw();
		ws.Flush();
		User::After(500000);
		}
	}

GLDEF_C TInt E32Main()
	{
	CTrapCleanup* cleanup = CTrapCleanup::New();
	RDebug::Printf("PH: land start");
	TRAPD(err, MainL());
	RDebug::Printf("PH: land exit err=%d", err);
	delete cleanup;
	return err;
	}
