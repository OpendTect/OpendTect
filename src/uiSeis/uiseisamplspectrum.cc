/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseisamplspectrum.h"

#include "arrayndimpl.h"
#include "seisdatapack.h"


uiSeisAmplSpectrum::uiSeisAmplSpectrum( uiParent* p,
					const uiAmplSpectrum::Setup& su )
    : uiAmplSpectrum(p,su)
{}


uiSeisAmplSpectrum::~uiSeisAmplSpectrum()
{}


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
