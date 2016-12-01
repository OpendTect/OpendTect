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


void uiSeisAmplSpectrum::setDataPackID( DataPack::ID dpid,
					DataPackMgr::ID dmid )
{
    uiAmplSpectrum::setDataPackID( dpid, dmid );

    if ( dmid == DataPackMgr::SeisID() )
    {
	ConstRefMan<DataPack> datapack = DPM(dmid).get( dpid );
	mDynamicCastGet(const VolumeDataPack*,vdp,datapack.ptr());
	if ( vdp )
	{
	    setup_.nyqvistspspace_ = vdp->getZRange().step;
	    setData( vdp->data() );
	}
    }
}
