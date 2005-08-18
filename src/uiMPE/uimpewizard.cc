/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.cc,v 1.10 2005-08-18 14:44:16 cvskris Exp $
________________________________________________________________________

-*/


#include "uimpewizard.h"

#include "emmanager.h"
#include "emobject.h"
#include "emtracker.h"
#include "executor.h"
#include "mpesetup.h"
#include "mpeengine.h"
#include "uimpesetup.h"
#include "uimpepartserv.h"

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

int Wizard::defcolnr		= 0;

const int Wizard::sNamePage		= 0;
const int Wizard::sSeedPage		= 1;
const int Wizard::sSetupPage		= 2;
const int Wizard::sFinalizePage		= 3;


Wizard::Wizard( uiParent* p, uiMPEPartServer* mps )
    : uiWizard(p,uiDialog::Setup("Tracking Wizard","XXXXXXX tracking","")
				.modal(false))
    , mpeserv(mps)
    , curtrackid(-1)
    , pickmode(false)
    , sid( -1 )
    , dosave( false )
{
    setHelpID( "108.0.0" );

    addPage( createNamePage() );
    addPage( createSeedPage() );
    addPage( createSetupPage() );
    addPage( createFinalizePage() );
}


Wizard::~Wizard()
{
}


uiGroup* Wizard::createNamePage()
{
    uiGroup* grp = new uiGroup( this, "Page 1" );
    namefld = new uiGenInput( grp, "Name" );
    return grp;
}


bool Wizard::prepareNamePage()
{
    namefld->setFocus();
    return true;
}

#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool Wizard::leaveNamePage(bool process)
{
    if ( !process ) return true;

    const char* newobjnm = namefld->text();
    if ( !*newobjnm )
	mErrRet( "Please provide name" )
    if ( newObjectPresent(newobjnm) )
    {
	if ( !uiMSG().askGoOn("An object with that name does already exist."
			      " Overwrite?",true) )
	    return false;
    }

    dosave = true;

/*     if ( engine().interactionseeds.size() )
    {
	displayPage(1,false); // skip seedpicking step
	return addTracker( newobjnm );
    }
 */
    colorfld->setColor( Color::drawDef(defcolnr) );
    return true;
}


uiGroup* Wizard::createSeedPage()
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
    
    markerszbox = new uiLabeledSpinBox( grp, "Size" );
    markerszbox->attach( rightTo, colorfld );
    markerszbox->box()->setValue( MPE::engine().seedsize );
    markerszbox->box()->valueChanged.notify( mCB(this,Wizard,stickSetChange) );

    linewidthbox = new uiLabeledSpinBox( grp, "Line width" );
    linewidthbox->attach( rightTo, markerszbox );
    linewidthbox->box()->setInterval(1,10,1);
    linewidthbox->box()->setValue( MPE::engine().seedlinewidth );
    linewidthbox->box()->valueChanged.notify( mCB(this,Wizard,stickSetChange) );

    return grp;
}


bool Wizard::prepareSeedPage()
{
    mpeserv->sendEvent( uiMPEPartServer::evStartSeedPick );
    stickSetChange(0);
    pickmode = true;

    return true;
}


bool Wizard::leaveSeedPage(bool process)
{
    mpeserv->sendEvent( uiMPEPartServer::evEndSeedPick );
    pickmode = false;

    if ( !process ) return true;

    if ( !engine().interactionseeds.size() )
    {
	mpeserv->sendEvent( uiMPEPartServer::evStartSeedPick );
	pickmode = true;
	mErrRet( "You did not create any seedpoints" );
    }

    if ( curtrackid==-1 )
    {
	curtrackid = mpeserv->addTracker( trackertype, namefld->text() );
	if ( curtrackid<0 )
	{
	    pErrMsg( "Could not create tracker" );
	    return false;
	}
	defcolnr++;

    }
    else
    {
	EMTracker* tracker = MPE::engine().getTracker(curtrackid);
	tracker->setSeeds( MPE::engine().interactionseeds,
			   tracker->objectName(), sid );
    }

    if ( dosave )
    {
	EMTracker* tracker = MPE::engine().getTracker(curtrackid);
	const EM::ObjectID objid = tracker->objectID();
	EM::EMObject* emobj = EM::EMM().getObject(objid);
	PtrMan<Executor> saver = emobj->saver();
	if ( saver ) 
	    saver->execute();
    }

    return true;
}


uiGroup* Wizard::createSetupPage()
{
    setupgrp = new uiSetupSel( this, mpeserv->attrset );
    return setupgrp;
}


bool Wizard::prepareSetupPage()
{
    EMTracker* tracker = MPE::engine().getTracker(curtrackid);
    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject(objid);
    if ( sid==-1 )
        sid = emobj->sectionID(emobj->nrSections()-1);

    displayPage(sNamePage, false );
    setupgrp->setType( objid, sid );

    return true;
}


bool Wizard::leaveSetupPage(bool process)
{
    if ( !process ) return true;
    if ( !setupgrp->processInput() )
	mErrRet( "Please select Tracking Setup" );

    mpeserv->updateVolumeFromSeeds();

    return true;
}


uiGroup* Wizard::createFinalizePage()
{
    uiGroup* grp = new uiGroup( this, "Page 4" );
    uiLabel* lbl = new uiLabel( grp, "Do you want to track another surface?" );
    anotherfld = new uiGenInput( grp, "", BoolInpSpec() );
    anotherfld->attach( alignedBelow, lbl );
    anotherfld->valuechanged.notify( mCB(this,Wizard,anotherSel) );

    BufferStringSet trackernames;
    engine().getAvaliableTrackerTypes( trackernames );
    typefld = new uiGenInput( grp, "Type", StringListInpSpec(trackernames) );
    typefld->attach( alignedBelow, anotherfld );
    anotherSel(0);
    return grp;
}


bool Wizard::leaveFinalizePage(bool process)
{
    if ( !process ) return true;

    if ( anotherfld->getBoolValue() )
    {
	setTrackingType( typefld->text() );
    }
    else
    {
	mpeserv->createActiveVolume();
	mpeserv->loadAttribData();
    }

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
    engine().seedlinewidth = linewidthbox->box()->getValue();
    engine().seedpropertychange.trigger();
}


bool Wizard::preparePage( int page )
{
    switch ( page )
    {
	case 0: return prepareNamePage();
	case 1: return prepareSeedPage();
	case 2: return prepareSetupPage();
	case 3: return prepareFinalizePage();
    }

    return true;
}


bool Wizard::leavePage( int page, bool process )
{
    switch ( page )
    {
	case 0: return leaveNamePage(process);
	case 1: return leaveSeedPage(process);
	case 2: return leaveSetupPage(process);
	case 3: return leaveFinalizePage(process);
    }

    return true;
}


bool Wizard::newObjectPresent( const char* objnm ) const
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    PtrMan<IOObj> ioobj = IOM().getLocal( objnm );
    return ioobj;
}


void Wizard::setTrackerID( int trackerid )
{
    curtrackid = trackerid;
    displayPage( sNamePage, false );
    displayPage( sFinalizePage, false );
    setRotateMode( false );
}


void Wizard::setSurfaceColor( const Color& col )
{
    colorfld->setColor( col );
}


void Wizard::reset()
{
    dosave = false;
    sid = -1;
    defcolnr++;
    curtrackid = -1;

    for ( int idx=0; idx<nrPages(); idx++ )
	displayPage( idx, true );
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
