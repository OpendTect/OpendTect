/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.cc,v 1.1 2005-03-11 16:59:03 cvsnanne Exp $
________________________________________________________________________

-*/


#include "uimpewizard.h"
#include "mpesetup.h"
#include "mpeengine.h"
#include "uimpesetup.h"
#include "uimpepartserv.h"
#include "emmanager.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "draw.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uispinbox.h"


namespace MPE {

int Wizard::defcolnr = 0;


Wizard::Wizard( uiParent* p, uiMPEPartServer* mps )
    : uiWizard(p,uiDialog::Setup("Tracking Wizard","XXXXXXX tracking","")
				.modal(false))
    , mpeserv(mps)
    , currentfinished(false)
    , curmid("")
{
    setHelpID( "108.0.0" );

    cancel.notify( mCB(this,Wizard,cancelWizard) );
    next.notify( mCB(this,Wizard,nextPage) );
    finish.notify( mCB(this,Wizard,finishWizard) );
    addPage( createPage1() );
    addPage( createPage2() );
    addPage( createPage3() );
    addPage( createPage4() );
}


Wizard::~Wizard()
{
}


uiGroup* Wizard::createPage1()
{
    uiGroup* grp = new uiGroup( this, "Page 1" );
    namefld = new uiGenInput( grp, "Name" );
    return grp;
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool Wizard::processPage1()
{
    const char* newobjnm = namefld->text();
    if ( !*newobjnm )
	mErrRet( "Please provide name" )

    int trackid = mpeserv->addTracker( trackertype, newobjnm );
    MultiID mid = mpeserv->getTrackerMultiID( trackid );
    EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    setupgrp->setType( objid, 0 );

    curmid = mid;
    currentfinished = false;

    colorfld->setColor( Color::drawDef(defcolnr++) );
    mpeserv->sendEvent( uiMPEPartServer::evStartSeedPick );
    stickSetChange(0);
    return true;
}


uiGroup* Wizard::createPage2()
{
    uiGroup* grp = new uiGroup( this, "Page 2" );

    BufferString str( "In this step you'll have to create your seedpoints.\n"
		      "Create a seedpoint by clicking on a slice.\n"
		      "You can remove a seedpoint by ctrl-click." );
    uiLabel* lbl = new uiLabel( grp, str );

    colorfld = new uiColorInput( grp, Color::drawDef(defcolnr++),
	    			 "Select ...", "Line color" );
    colorfld->colorchanged.notify( mCB(this,Wizard,stickSetChange) );
    colorfld->attach( alignedBelow, lbl );
    
    markerszbox = new uiLabeledSpinBox( grp, "size" );
    markerszbox->attach( rightTo, colorfld );
    markerszbox->box()->setValue( 3 );
    markerszbox->box()->valueChanged.notify( mCB(this,Wizard,stickSetChange) );

    return grp;
}


bool Wizard::processPage2()
{
    if ( !engine().interactionseeds.size() )
    {
	uiMSG().error( "You did not create any seedpoints" );
	return false;
    }

    return true;
}


uiGroup* Wizard::createPage3()
{
    setupgrp = new uiSetupSel( this, mpeserv->attrset );
    return setupgrp;
}


bool Wizard::processPage3()
{
    if ( !setupgrp->processInput() )
	mErrRet( "Please select Tracking Setup" );

    return true;
}


uiGroup* Wizard::createPage4()
{
    uiGroup* grp = new uiGroup( this, "Page 4" );
    uiLabel* lbl = new uiLabel( grp, "Do you want to track another surface?" );
    anotherfld = new uiGenInput( grp, "", BoolInpSpec() );
    anotherfld->attach( alignedBelow, lbl );
    anotherfld->valuechanged.notify( mCB(this,Wizard,anotherSel) );

    BufferStringSet trackernames;
    MPE::engine().getAvaliableTrackerTypes( trackernames );
    typefld = new uiGenInput( grp, "Type", StringListInpSpec(trackernames) );
    typefld->attach( alignedBelow, anotherfld );
    anotherSel(0);
    return grp;
}


bool Wizard::processPage4()
{
    currentfinished = true;
    if ( anotherfld->getBoolValue() )
    {
	setTrackingType( typefld->text() );
	displayPage(0,true);
	displayPage(1,true);
	displayPage(2,true);
    }

//  mpeserv->sendEvent( uiMPEPartServer::evFinishInit );
    return true;
}


void Wizard::anotherSel( CallBacker* )
{
    const bool cont = anotherfld->getBoolValue();
    typefld->display( cont );
    setRotateMode( cont );
}


void Wizard::stickSetChange( CallBacker* )
{
    engine().seedcolor = colorfld->color();
    engine().seedsize = markerszbox->box()->getValue();
    engine().seedpropertychange.trigger();
}


void Wizard::nextPage( CallBacker* )
{
    const int pageidx = currentPageIdx();
    bool res = true;
    switch( pageidx )
    {
	case 0: res = processPage1(); break;
	case 1: res = processPage2(); break;
	case 2: res = processPage3(); break;
	case 3: res = processPage4(); break;
    }

    approvePage( res );
}


void Wizard::cancelWizard( CallBacker* )
{
    if ( !*curmid ) return;

/*
    if ( !currentfinished )
	mpeserv->removeObject( curmid );

    mpeserv->finishAll();
*/
}


void Wizard::finishWizard( CallBacker* )
{
    nextPage(0);
//  mpeserv->finishAll();
}


void Wizard::setSurfaceColor( const Color& col )
{
    colorfld->setColor( col );
}


void Wizard::startAt( int startidx )
{
    for ( int idx=0; idx<nrPages(); idx++ )
	displayPage( idx, idx>=startidx );
}


void Wizard::setTrackingType( const char* tp )
{
    trackertype = tp;
    updateDialogTitle();
}


void Wizard::updateDialogTitle()
{
    BufferString str( trackertype ); str += " Tracking";
    setTitleText( str );
}

}; // namespace MPE
