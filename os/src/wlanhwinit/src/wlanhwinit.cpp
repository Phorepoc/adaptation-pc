// Copyright The Phorepoc Authors
// SPDX-License-Identifier: Apache-2.0

// wlanhwinit.cpp -- the WLAN hardware-init plug-in of a board without WLAN hardware. The WLAN engine asks it for the chip's
// initialisation and firmware data and for its MAC address before it starts the chip; with no chip there is no data, no
// firmware and no address, and the address call fails (KErrNotFound) so the engine stops cleanly instead of running on an
// invented identity. The test-data calls are the engine's manufacturing interface: not supported.
#include <e32base.h>
#include <wlanhwinit.h>

CWlanHwInit::CWlanHwInit() : iMain( NULL )
    {
    }

void CWlanHwInit::ConstructL()
    {
    }

EXPORT_C CWlanHwInit* CWlanHwInit::NewL()
    {
    CWlanHwInit* self = new( ELeave ) CWlanHwInit;
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }

EXPORT_C CWlanHwInit::~CWlanHwInit()
    {
    }

EXPORT_C void CWlanHwInit::GetHwInitData( const TUint8** aInitData, TUint& aInitLength, const TUint8** aFwData, TUint& aFwLength )
    {
    *aInitData = NULL;
    aInitLength = 0;
    *aFwData = NULL;
    aFwLength = 0;
    }

EXPORT_C TInt CWlanHwInit::GetMacAddress( TMacAddr& aMacAddress )
    {
    for ( TInt i = 0; i < 6; ++i )
        {
        aMacAddress.iMacAddress[i] = 0;
        }
    return KErrNotFound;
    }

EXPORT_C void CWlanHwInit::GetHwTestInitData( const TUint8** aInitData, TUint& aInitLength, const TUint8** aFwData, TUint& aFwLength )
    {
    GetHwInitData( aInitData, aInitLength, aFwData, aFwLength );
    }

EXPORT_C TInt CWlanHwInit::GetHwTestData( TUint /*aId*/, TDes8& /*aData*/ )
    {
    return KErrNotSupported;
    }

EXPORT_C TInt CWlanHwInit::SetHwTestData( TUint /*aId*/, TDesC8& /*aData*/ )
    {
    return KErrNotSupported;
    }
