/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uivelocitygridder.cc,v 1.1 2008-07-22 19:44:22 cvskris Exp $";

#include "uivelocitygridder.h"

#include "uiselectvelocityfunction.h"
#include "uigridder2d.h"
#include "gridder2d.h"
#include "uilabel.h"
#include "uivolprocchain.h"
#include "velocitygridder.h"
#include "volprocchain.h"

namespace VolProc
{

void uiVelocityGridder::initClass()
{
    VolProc::uiPS().addCreator( create, VelGriddingStep::sType() );
}


uiDialog* uiVelocityGridder::create( uiParent* p, VolProc::Step* ro )
{
    mDynamicCastGet( VelGriddingStep*, gridop, ro );
    if ( !gridop ) return 0;

    return new uiVelocityGridder(p,gridop);
}


uiVelocityGridder::uiVelocityGridder( uiParent* p, VelGriddingStep* ro )
    : uiDialog( p, uiDialog::Setup("Gridding parameters",0,"dgb:104.1.2") )
    , operation_( ro )
{
    griddersel_ = new uiGridder2DSel( this, ro->getGridder() );

    uiLabel* label = new uiLabel( this, "Velocity sources" );
    label->attach( alignedBelow, griddersel_ );

    velfuncsel_ = new Vel::uiFunctionSel( this, operation_->getPicks(), 0 );
    velfuncsel_->attach( alignedBelow, label );
}


uiVelocityGridder::~uiVelocityGridder()
{}


bool uiVelocityGridder::acceptOK(CallBacker*)
{
    if ( !operation_ ) return true;
    operation_->setPicks( velfuncsel_->getVelSources() );
    operation_->setGridder( griddersel_->getSel()->clone() );
    return true;
}



}; //namespace
