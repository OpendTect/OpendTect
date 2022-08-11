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
		DataPackID dpid, DataPackMgr::MgrID dmid, int version )
{
    uiAmplSpectrum::setDataPackID( dpid, dmid, version );

    if ( dmid == DataPackMgr::SeisID() )
    {
	auto dp = DPM(dmid).get<SeisDataPack>( dpid );
	if ( dp )
	{
	    setup_.nyqvistspspace_ = dp->zRange().step;
	    setData( dp->data(version) );
	}
    }
}
