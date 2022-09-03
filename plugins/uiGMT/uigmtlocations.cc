/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtlocations.h"

#include "ctxtioobj.h"
#include "gmtpar.h"
#include "ioman.h"
#include "ioobj.h"
#include "picksettr.h"

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
    : uiGMTOverlayGrp(p,uiStrings::sLocation(mPlural))
    , ctio_(*mMkCtxtIOObj(PickSet))
{
    inpfld_ = new uiIOObjSel( this, ctio_,uiStrings::sPointSet() );
    inpfld_->selectionDone.notify( mCB(this,uiGMTLocationsGrp,objSel) );

    namefld_ = new uiGenInput( this, uiStrings::sName(), StringInpSpec() );
    namefld_->setElemSzPol( uiObject::Wide );
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

    IOObj* ioobj = ctio_.ioobj_;
    if ( ioobj )
	namefld_->setText( ioobj->name() );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTLocationsGrp::fillPar( IOPar& par ) const
{
    if ( !inpfld_->commitInput() || !ctio_.ioobj_ )
	mErrRet(tr("Please select a pickset"))

    inpfld_->fillPar( par );
    par.set( sKey::Name(), namefld_->text() );
    return symbfld_->fillPar( par );
}


bool uiGMTLocationsGrp::usePar( const IOPar& par )
{
    inpfld_->usePar( par );
    const BufferString nm = par.find( sKey::Name() );
    if ( !nm.isEmpty() )
	namefld_->setText( nm );

    return symbfld_->usePar( par );
}
