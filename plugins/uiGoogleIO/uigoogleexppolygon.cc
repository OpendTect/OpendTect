/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2007
-*/


#include "uigoogleexppolygon.h"

#include "googlexmlwriter.h"
#include "ioman.h"
#include "latlong.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "pickset.h"
#include "picksettr.h"
#include "strmprov.h"
#include "survinfo.h"

#include "uisellinest.h"
#include "uigeninput.h"
#include "uifileinput.h"
#include "uiioobjselgrp.h"
#include "uimsg.h"
#include "uiseparator.h"

#include <iostream>


uiGISExportPolygon::uiGISExportPolygon( uiParent* p, const MultiID& mid )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport( tr("Polygon to GIS")),
				 tr("Specify output parameters"),
                                 mODHelpKey(mGoogleExportPolygonHelpID) ) )
{
    uiIOObjSelGrp::Setup su;
    su.choicemode_ = OD::ChooseZeroOrMore;
    selfld_ = new uiIOObjSelGrp( this, mIOObjContext(PickSet),
						uiStrings::sSelect(), su );

    auto* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, selfld_ );
    expfld_ = new uiGISExpStdFld( this, "pickpoly" );
    expfld_->attach( stretchedBelow, sep );
    expfld_->attach( leftAlignedBelow, selfld_ );
}

bool uiGISExportPolygon::acceptOK( CallBacker* )
{
    TypeSet<MultiID> objids;
    selfld_->getChosen( objids );
    if ( objids.isEmpty() )
    {
	uiMSG().error(
		    uiStrings::phrPlsSelectAtLeastOne(uiStrings::sObject()) );
	return false;
    }

    Pick::Set picks;
    BufferString errmsg;
    PtrMan<GISWriter> wrr = expfld_->createWriter();
    if ( !wrr )
	return false; // Put some error message here

    ObjectSet<const Pick::Set> pickssets;
    for ( auto objid : objids )
    {
	picks.setEmpty();
	if ( !PickSetTranslator::retrieve(
					picks,IOM().get(objid),true,errmsg) )
	    continue;

	if ( picks.isPolygon() )
	{
	    TypeSet<Coord3> coords;
	    ObjectSet<const Pick::Location> locs;
	    picks.getLocations( locs );
	    if ( locs.isEmpty() )
		continue;

	    for ( auto loc : locs )
		coords += loc->pos();

	    coords += picks.first().pos();
	    wrr->writePolygon( coords, picks.getName() );
	}
	else
	    pickssets.add( new Pick::Set(picks) );
    }

    if ( !picks.isPolygon() )
	wrr->writePoint( pickssets );

    wrr->close();
    const bool ret = uiMSG().askGoOn( wrr->successMsg() );
    return !ret;
}
