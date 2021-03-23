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

    Pick::Set pickset;
    BufferString errmsg;
    PtrMan<GISWriter> wrr = expfld_->createWriter();
    if ( !wrr )
	return false; // Put some error message here

    ObjectSet<const Pick::Set> picks;
    for ( auto objid : objids )
    {
	pickset.setEmpty();
	if ( !PickSetTranslator::retrieve(
					pickset,IOM().get(objid),true,errmsg) )
	    continue;

	if ( pickset.isPolygon() )
	{
	    TypeSet<Coord3> coords;
	    ObjectSet<const Pick::Location> locs;
	    pickset.getLocations( locs );
	    if ( locs.isEmpty() )
		continue;

	    for ( auto loc : locs )
		coords += loc->pos();

	    coords += pickset.first().pos();
	    wrr->writePolygon( coords, pickset.getName() );
	}
	else
	    picks.add( new Pick::Set(pickset) );
    }

    if ( !pickset.isPolygon() )
	wrr->writePoint( picks );

    wrr->close();
    const bool ret = uiMSG().askGoOn( wrr->successMsg() );
    return !ret;
}
