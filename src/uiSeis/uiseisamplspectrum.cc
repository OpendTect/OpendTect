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
	ConstDataPackRef<DataPack> datapack = DPM(dmid).obtain( dpid );
	mDynamicCastGet(const SeisDataPack*,dp,datapack.ptr());
	if ( dp )
	{
	    setup_.nyqvistspspace_ = dp->getZRange().step;
	    setData( dp->data(version) );
	}
    }
}
