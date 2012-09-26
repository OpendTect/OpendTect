/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		July 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uigmtlocations.h"

#include "ctxtioobj.h"
#include "gmtpar.h"
#include "ioobj.h"
#include "ioman.h"
#include "picksettr.h"
#include "pixmap.h"
#include "uigeninput.h"
#include "uigmtsymbolpars.h"
#include "uiioobjsel.h"
#include "uimsg.h"


int uiGMTLocationsGrp::factoryid_ = -1;

void uiGMTLocationsGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Locations",
				    uiGMTLocationsGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTLocationsGrp::createInstance( uiParent* p )
{
    return new uiGMTLocationsGrp( p );
}


uiGMTLocationsGrp::uiGMTLocationsGrp( uiParent* p )
    : uiGMTOverlayGrp(p,"Locations")
    , ctio_(*mMkCtxtIOObj(PickSet))
{
    inpfld_ = new uiIOObjSel( this, ctio_,"Pick Set" );
    inpfld_->selectionDone.notify( mCB(this,uiGMTLocationsGrp,objSel) );

    namefld_ = new uiGenInput( this, "Name", StringInpSpec() );
    namefld_->attach( alignedBelow, inpfld_ );

    symbfld_ = new uiGMTSymbolPars( this, false );
    symbfld_->attach( alignedBelow, namefld_ );
}


void uiGMTLocationsGrp::reset()
{
    inpfld_->clear();
    namefld_->clear();
    symbfld_->reset();
}


void uiGMTLocationsGrp::objSel( CallBacker* )
{
    if ( !inpfld_->commitInput() )
	return;

    IOObj* ioobj = ctio_.ioobj;
    if ( ioobj ) 
	namefld_->setText( ioobj->name() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTLocationsGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj )
	mErrRet("Please select a pickset")

    inpfld_->fillPar( par );
    par.set( sKey::Name(), namefld_->text() );
    return symbfld_->fillPar( par );
}


bool uiGMTLocationsGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    const char* nm = par.find( sKey::Name() );
    if ( nm && *nm ) namefld_->setText( nm );

    return symbfld_->usePar( par );
}

