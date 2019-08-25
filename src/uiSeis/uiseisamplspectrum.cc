/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2007
_______________________________________________________________________

-*/

#include "uiseisamplspectrum.h"

#include "arrayndimpl.h"
#include "seisdatapack.h"


void uiSeisAmplSpectrum::setDataPackID(
		DataPack::ID dpid, DataPackMgr::ID dmid, int version )
{
    uiAmplSpectrum::setDataPackID( dpid, dmid, version );

    if ( dmid == DataPackMgr::SeisID() )
    {
	auto vdp = DPM(dmid).get<VolumeDataPack>( dpid );
	if ( vdp )
	{
	    setup_.nyqvistspspace_ = vdp->zRange().step;
	    setData( vdp->data(version) );
	}
    }
}
