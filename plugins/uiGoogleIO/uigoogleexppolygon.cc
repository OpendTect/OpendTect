/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
				 mODHelpKey(mGoogleExportPolygonHelpID)) )
    , mid_(mid)
{
    uiIOObjSelGrp::Setup su;
    su.choicemode_ = OD::ChooseZeroOrMore;
    expfld_ = new uiGISExpStdFld( this, "pickpoly" );

    if ( mid.isUdf() )
    {
	inputyp_ = new uiGenInput( this,
				    uiStrings::phrInput(uiStrings::sType()),
		BoolInpSpec(true,uiStrings::sPickSet(),uiStrings::sPolygon()) );
	mAttachCB(inputyp_->valuechanged,uiGISExportPolygon::inputTypChngCB);
	selfld_ = new uiIOObjSelGrp( this, mIOObjContext(PickSet),
						uiStrings::sSelect(), su );
	selfld_->attach( alignedBelow, inputyp_ );
	auto* sep = new uiSeparator( this );
	sep->attach( stretchedBelow, selfld_ );
	expfld_->attach( stretchedBelow, sep );
	expfld_->attach( leftAlignedBelow, selfld_ );
	mAttachCB(postFinalize(),uiGISExportPolygon::inputTypChngCB);
    }
}


uiGISExportPolygon::~uiGISExportPolygon()
{
    detachAllNotifiers();
}


void uiGISExportPolygon::inputTypChngCB( CallBacker* )
{
    const bool ispickset = inputyp_->getBoolValue();
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->ctxt_.forread_ = true;
    PickSetTranslator::fillConstraints( ctio->ctxt_, !ispickset );
    selfld_->setContext( ctio->ctxt_ );
    selfld_->fullUpdate( 0 );

}


bool uiGISExportPolygon::acceptOK( CallBacker* )
{
    TypeSet<MultiID> objids;
    if ( selfld_ )
    {
	selfld_->getChosen( objids );
	if ( objids.isEmpty() )
	{
	    uiMSG().error(
		    uiStrings::phrPlsSelectAtLeastOne(uiStrings::sObject()) );
	    return false;
	}
    }
    else
	objids.add( mid_ );

    PtrMan<GISWriter> wrr = expfld_->createWriter();
    if ( !wrr )
	return false; // Put some error message here

    BufferString errmsg;
    RefObjectSet<const Pick::Set> selsets;
    for ( auto objid : objids )
    {
	RefMan<Pick::Set> ps = new Pick::Set;
	if ( !PickSetTranslator::retrieve(*ps,IOM().get(objid),true,errmsg) )
	    continue;

	selsets.add( ps );
    }

    if ( selsets.isEmpty() )
	return false;

    const Pick::Set* ps0 = selsets.first();
    const bool ispoly = ps0->isPolygon();
    if ( ispoly )
    {
	if ( ps0->disp_.connect_ == Pick::Set::Disp::Close )
	    wrr->writePolygon( selsets );
	else
	    wrr->writeLine( selsets );
    }
    else
	wrr->writePoint( selsets );

    wrr->close();
    const bool ret = uiMSG().askGoOn( wrr->successMsg() );
    return !ret;
}
