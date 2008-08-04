/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocitygridder.cc,v 1.2 2008-08-04 22:31:16 cvskris Exp $";

#include "uivelocitygridder.h"

#include "uiselectvelocityfunction.h"
#include "uigridder2d.h"
#include "gridder2d.h"
#include "uilabel.h"
#include "uivolprocchain.h"
#include "uigeninput.h"
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
    : uiStepDialog( p, uiDialog::Setup("Gridding parameters",0,"dgb:104.1.2"),
	            ro )
    , operation_( ro )
{
    griddersel_ = new uiGridder2DSel( this, ro->getGridder() );
    griddersel_->attach( alignedBelow, namefld_ );

    uiLabel* label = new uiLabel( this, "Velocity sources" );
    label->attach( alignedBelow, griddersel_ );

    velfuncsel_ = new Vel::uiFunctionSel( this, operation_->getPicks(), 0 );
    velfuncsel_->attach( alignedBelow, label );
}


uiVelocityGridder::~uiVelocityGridder()
{}


bool uiVelocityGridder::acceptOK( CallBacker* cb )
{
    if ( !uiStepDialog::acceptOK( cb ) )
	return false;

    if ( !operation_ ) return true;
    operation_->setPicks( velfuncsel_->getVelSources() );
    operation_->setGridder( griddersel_->getSel()->clone() );
    return true;
}



}; //namespace
