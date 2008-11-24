/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocitygridder.cc,v 1.4 2008-11-24 15:55:11 cvskris Exp $";

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
    VolProc::uiChain::factory().addCreator( create, VelGriddingStep::sType() );
}


uiStepDialog* uiVelocityGridder::create( uiParent* p, VolProc::Step* ro )
{
    mDynamicCastGet( VelGriddingStep*, gridop, ro );
    if ( !gridop ) return 0;

    return new uiVelocityGridder(p,gridop);
}


uiVelocityGridder::uiVelocityGridder( uiParent* p, VelGriddingStep* ro )
    : uiStepDialog( p, uiDialog::Setup(VelGriddingStep::sUserName(),
		VelGriddingStep::sUserName(),"dgb:104.1.2"),
	   	    ro )
    , operation_( ro )
{
    griddersel_ = new uiGridder2DSel( this, ro->getGridder() );
    griddersel_->attach( alignedBelow, namefld_ );

    uiLabel* label = new uiLabel( this, "Velocity sources" );
    label->attach( alignedBelow, griddersel_ );

    velfuncsel_ = new Vel::uiFunctionSel( this, operation_->getSources(), 0 );
    velfuncsel_->attach( alignedBelow, label );
}


uiVelocityGridder::~uiVelocityGridder()
{}


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
