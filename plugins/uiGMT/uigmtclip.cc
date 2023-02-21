/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtclip.h"

#include "ctxtioobj.h"
#include "draw.h"
#include "gmtpar.h"
#include "pickset.h"
#include "picksettr.h"
#include "uigeninput.h"
#include "uiioobjsel.h"


int uiGMTClipGrp::factoryid_ = -1;

void uiGMTClipGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Clipping",
				    uiGMTClipGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTClipGrp::createInstance( uiParent* p )
{
    return new uiGMTClipGrp( p );
}


uiGMTClipGrp::uiGMTClipGrp( uiParent* p )
    : uiGMTOverlayGrp(p,tr("Clipping"))
{
    actionfld_ = new uiGenInput(this, uiString::emptyString(),
                                BoolInpSpec(true,tr("Start clipping"),
				tr("Stop clipping"),true));

    IOObjContext ctxt( PickSetTranslatorGroup::ioContext() );
    ctxt.toselect_.require_.set( sKey::Type(), sKey::Polygon() );
    polygonfld_ = new uiIOObjSel( this, ctxt, uiStrings::sPolygon() );
    polygonfld_->attach( alignedBelow, actionfld_ );

    optionfld_ = new uiGenInput(this, uiString::emptyString(),
	    BoolInpSpec(true,tr("Clip Outside"),tr("Clip Inside"),true) );
    optionfld_->attach( alignedBelow, polygonfld_ );
    actionfld_->valueChanged.notify( mCB(this,uiGMTClipGrp,actionSel) );
    postFinalize().notify( mCB(this,uiGMTClipGrp,actionSel) );
}


void uiGMTClipGrp::reset()
{
    polygonfld_->clear();
    actionfld_->setValue( true );
    optionfld_->setValue( true );
    actionSel( 0 );
}


void uiGMTClipGrp::actionSel( CallBacker* )
{
    const bool isstartofclipping = actionfld_->getBoolValue();
    polygonfld_->display( isstartofclipping );
    optionfld_->display( isstartofclipping );
}


bool uiGMTClipGrp::fillPar( IOPar& par ) const
{
    const bool isstartofclipping = actionfld_->getBoolValue();
    par.setYN( ODGMT::sKeyStartClipping(), isstartofclipping );
    if ( !isstartofclipping )
	return true;

    if ( !polygonfld_->ioobj() )
	return false;

    polygonfld_->fillPar( par );
    par.setYN( ODGMT::sKeyClipOutside(), optionfld_->getBoolValue() );
    return true;
}


bool uiGMTClipGrp::usePar( const IOPar& par )
{
    bool isstartofclipping = false;
    par.getYN( ODGMT::sKeyStartClipping(), isstartofclipping );
    actionfld_->setValue( isstartofclipping );
    if ( isstartofclipping )
    {
	polygonfld_->usePar( par );
	bool clipoutside = true;
	par.getYN( ODGMT::sKeyClipOutside(), clipoutside );
	optionfld_->setValue( clipoutside );
    }

    actionSel( 0 );
    return true;
}


bool uiGMTClipGrp::getTerminatingPars( IOPar& par )
{
    par.setYN( ODGMT::sKeyStartClipping(), false );
    return true;
}
