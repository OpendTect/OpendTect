/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		September 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uigmtadv.cc,v 1.6 2011/04/01 09:44:21 cvsbert Exp $";

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
    : uiGMTOverlayGrp(p,"Advanced")
{
    uiLabel* lbl = new uiLabel( this, "Customized GMT command" );
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
	mErrRet("Please enter a valid GMT command")

    par.set( ODGMT::sKeyCustomComm, comm );
    return true;
}


bool uiGMTAdvGrp::usePar( const IOPar& par )
{
    const char* comm = par.find( ODGMT::sKeyCustomComm );
    inpfld_->setvalue_( comm );
    return true;
}

