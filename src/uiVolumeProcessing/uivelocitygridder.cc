/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocitygridder.cc,v 1.12 2011/08/24 13:19:43 cvskris Exp $";

#include "uivelocitygridder.h"

#include "uiselectvelocityfunction.h"
#include "uigridder2d.h"
#include "uigeninput.h"
#include "gridder2d.h"
#include "uilabel.h"
#include "uivolprocchain.h"
#include "velocitygridder.h"
#include "velocityfunction.h"
#include "volprocchain.h"

namespace VolProc
{

uiStepDialog* uiVelocityGridder::createInstance( uiParent* p, VolProc::Step* ro )
{
    mDynamicCastGet( VelGriddingStep*, gridop, ro );
    if ( !gridop ) return 0;

    return new uiVelocityGridder(p,gridop);
}


uiVelocityGridder::uiVelocityGridder( uiParent* p, VelGriddingStep* ro )
    : uiStepDialog( p, VelGriddingStep::sFactoryDisplayName(), ro )
    , operation_( ro )
{
    setHelpID( "103.6.5" );

    griddersel_ = new uiGridder2DSel( this, ro->getGridder() );

    uiLabel* label = new uiLabel( this, "Velocity sources" );
    label->attach( alignedBelow, griddersel_ );

    velfuncsel_ = new Vel::uiFunctionSel( this, operation_->getSources(), 0 );
    velfuncsel_->attach( alignedBelow, label );
    velfuncsel_->listChange.notify( mCB(this,uiVelocityGridder,sourceChangeCB));

    addNameFld( velfuncsel_ );

    namenotset_ = namefld_->isUndef();
    if ( namenotset_ )
    {
	namefld_->valuechanged.notify(
		mCB(this,uiVelocityGridder,nameChangeCB) );
    }
}


void uiVelocityGridder::nameChangeCB( CallBacker* )
{
    namenotset_ = false;
}


void uiVelocityGridder::sourceChangeCB( CallBacker* )
{
    if ( namenotset_ && velfuncsel_->getVelSources().size()==1 )
    {
	namefld_->valuechanged.remove(mCB(this,uiVelocityGridder,nameChangeCB));
	namefld_->setText( velfuncsel_->getVelSources()[0]->userName() );
	namefld_->valuechanged.notify(mCB(this,uiVelocityGridder,nameChangeCB));
    }
}



bool uiVelocityGridder::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    if ( !operation_ ) return true;

    operation_->setSources( velfuncsel_->getVelSources() );
    operation_->setGridder( griddersel_->getSel()->clone() );

    return true;
}


}; //namespace
