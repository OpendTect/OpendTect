/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		September 2008
 RCS:		$Id: uigmtadv.cc,v 1.1 2008-09-12 11:32:30 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtadv.h"

#include "gmtpar.h"
#include "uilabel.h"
#include "uilineedit.h"


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
    uiLabel* lbl = new uiLabel( this, "Add your customized GMT command" );
    inpfld_ = new uiLineEdit( this, "GMT Command" );
    inpfld_->setHSzPol( uiObject::WideMax );
    inpfld_->attach( alignedBelow, lbl );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGMTAdvGrp::fillPar( IOPar& par ) const
{
    const char* comm = inpfld_->getvalue_();
    par.set( ODGMT::sKeyCustomComm, comm );
    return true;
}


bool uiGMTAdvGrp::usePar( const IOPar& par )
{
    const char* comm = par.find( ODGMT::sKeyCustomComm );
    inpfld_->setvalue_( comm );
    return true;
}

