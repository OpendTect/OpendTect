/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocitygridder.cc,v 1.8 2011-03-23 14:47:28 cvsbruno Exp $";

#include "uivelocitygridder.h"

#include "uiselectvelocityfunction.h"
#include "uigridder2d.h"
#include "uigeninput.h"
#include "gridder2d.h"
#include "uilabel.h"
#include "uivolprocchain.h"
#include "velocitygridder.h"
#include "volprocchain.h"

namespace VolProc
{

void uiVelocityGridder::initClass()
{
    uiChain::factory().addCreator( create, VelGriddingStep::sType(),
	   "Velocity gridder" );
}


uiStepDialog* uiVelocityGridder::create( uiParent* p, VolProc::Step* ro )
{
    mDynamicCastGet( VelGriddingStep*, gridop, ro );
    if ( !gridop ) return 0;

    return new uiVelocityGridder(p,gridop);
}


uiVelocityGridder::uiVelocityGridder( uiParent* p, VelGriddingStep* ro )
    : uiStepDialog( p, VelGriddingStep::sUserName(), ro )
    , operation_( ro )
{
    setHelpID( "103.6.5" );

    griddersel_ = new uiGridder2DSel( this, ro->getGridder() );

    uiLabel* label = new uiLabel( this, "Velocity sources" );
    label->attach( alignedBelow, griddersel_ );

    velfuncsel_ = new Vel::uiFunctionSel( this, operation_->getSources(), 0 );
    velfuncsel_->attach( alignedBelow, label );

    addNameFld( velfuncsel_ );
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
