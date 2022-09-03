/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigmtadv.h"

#include "gmtpar.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimsg.h"


int uiGMTAdvGrp::factoryid_ = -1;

void uiGMTAdvGrp::initClass()
{
    if ( factoryid_ < 0 )
	factoryid_ = uiGMTOF().add( "Advanced",
				    uiGMTAdvGrp::createInstance );
}


uiGMTOverlayGrp* uiGMTAdvGrp::createInstance( uiParent* p )
{
    return new uiGMTAdvGrp( p );
}


uiGMTAdvGrp::uiGMTAdvGrp( uiParent* p )
    : uiGMTOverlayGrp(p,uiStrings::sAdvanced())
{
    uiLabel* lbl = new uiLabel( this, tr("Customized GMT command") );
    inpfld_ = new uiLineEdit( this, "GMT Command" );
    inpfld_->setHSzPol( uiObject::WideMax );
    inpfld_->attach( centeredBelow, lbl );
}


void uiGMTAdvGrp::reset()
{
    inpfld_->setvalue_( "" );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTAdvGrp::fillPar( IOPar& par ) const
{
    const char* comm = inpfld_->getvalue_();
    if ( !comm || !*comm )
	mErrRet(tr("Please enter a valid GMT command"))

    par.set( ODGMT::sKeyCustomComm(), comm );
    return true;
}


bool uiGMTAdvGrp::usePar( const IOPar& par )
{
    const BufferString comm = par.find( ODGMT::sKeyCustomComm() );
    inpfld_->setvalue_( comm );
    return true;
}
