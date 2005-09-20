/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.cc,v 1.21 2005-09-20 21:56:26 cvskris Exp $
________________________________________________________________________

-*/


#include "uimpewizard.h"

#include "ctxtioobj.h"
#include "emmanager.h"
#include "emobject.h"
#include "emseedpicker.h"
#include "ioman.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "ptrman.h"
#include "survinfo.h"
#include "uicolor.h"
#include "uicursor.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimpepartserv.h"
#include "uimpesetup.h"
#include "uiseparator.h"
#include "uispinbox.h"


namespace MPE {

int Wizard::defcolnr = 0;

const int Wizard::sNamePage		= 0;
const int Wizard::sSeedSetupPage	= 1;
const int Wizard::sFinalizePage	= 2;


Wizard::Wizard( uiParent* p, uiMPEPartServer* mps )
    : uiWizard(p,uiDialog::Setup("Tracking Wizard","XXXXXXX tracking","")
				.modal(false))
    , mpeserv(mps)
    , sid( -1 )
    , currentobject(-1)
    , objectcreated(false)
    , trackercreated(false)
    , reloadattribdata(false)
{
    addPage( createNamePage() );
    addPage( createSeedSetupPage() );
    addPage( createFinalizePage() );

    seedbox.setEmpty();
    setHelpID( "108.0.0" );
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


uiGroup* Wizard::createSeedSetupPage()
{
    uiGroup* grp = new uiGroup( this, "Page 2" );

    BufferString str( "In this step you'll have to create your seedpoints.\n"
		      "First select tracking setup, then\n"
		      "create a seedpoint by clicking on a slice.\n"
		      "You can remove a seedpoint by ctrl-click." );
    uiLabel* lbl = new uiLabel( grp, str );
    
    uiSeparator* sep1 = new uiSeparator( grp, "Separator 1" );
    sep1->attach( stretchedBelow, lbl );

    setupgrp = new uiSetupSel( grp, mpeserv->attrset );
    setupgrp->attach( alignedBelow, lbl );
    setupgrp->attach( ensureBelow, sep1 );
    setupgrp->setupchg.notify( mCB(this,Wizard,setupChange) );

    uiSeparator* sep2 = new uiSeparator( grp, "Separator 2" );
    sep2->attach( stretchedBelow, setupgrp );

    colorfld = new uiColorInput( grp, Color::drawDef(defcolnr++),
	    			 "Line color" );
    colorfld->colorchanged.notify( mCB(this,Wizard,stickSetChange) );
    colorfld->attach( alignedBelow, setupgrp );
    colorfld->attach( ensureBelow, sep2 );
    
    markerszbox = new uiLabeledSpinBox( grp, "Size" );
    markerszbox->attach( rightTo, colorfld );
    markerszbox->box()->setValue( MPE::engine().seedsize );
    markerszbox->box()->valueChanged.notify( mCB(this,Wizard,stickSetChange) );

    linewidthbox = new uiLabeledSpinBox( grp, "Line width" );
    linewidthbox->attach( rightTo, markerszbox );
    linewidthbox->box()->setInterval(1,10,1);
    linewidthbox->box()->setValue( MPE::engine().seedlinewidth );
    linewidthbox->box()->valueChanged.notify( 
	    				mCB(this,Wizard,stickSetChange) );
    return grp;
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


bool Wizard::prepareNamePage()
{
    namefld->setFocus();
    return true;
}

#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool Wizard::leaveNamePage( bool process )
{
    if ( !process ) return true;

    const char* newobjnm = namefld->text();
    if ( !*newobjnm )
	mErrRet( "Please provide name" );

    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id) );
    PtrMan<IOObj> ioobj = IOM().getLocal( newobjnm );
    if ( ioobj )
    {
	EM::ObjectID objid = EM::EMM().multiID2ObjectID( ioobj->key() );
	EM::EMObject* emobj = EM::EMM().getObject( objid );
	if ( emobj )
	{
	    uiMSG().error("An object with this name exist and is currently\n"
		    	  "loaded. Please select another name or quit the\n"
			  "wizard and remove the object with this name from\n"
			  "the tree.");
	    return false;
	}
	else
	{
	    if ( !uiMSG().askGoOn("An object with that name does already exist."
				  " Overwrite?",true) )
		return false;

	    /*
	    objid = EM::EMM().createObject(trackertype,newobjnm);
	    emobj = EM::EMM().getObject( objid );
	    currentobject = emobj->multiID();
	    objectcreated = true;
	    */
	}
    }
    else
	currentobject = -1;

    colorfld->setColor( Color::drawDef(defcolnr) );
    return true;
}


bool Wizard::prepareSeedSetupPage()
{
    if ( !(currentobject==-1) )
    {
	const EM::ObjectID objid = EM::EMM().multiID2ObjectID( currentobject );
	EM::EMObject* emobj = EM::EMM().getObject( objid );
	colorfld->setColor( emobj->preferredColor() );
    }

    if ( !createTracker() )
	return false;

    reloadattribdata = true;
    const int trackerid = mpeserv->getTrackerID( currentobject );

    const EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject(objid);
    if ( sid==-1 )
        sid = emobj->sectionID( emobj->nrSections()-1 );

    emobj->setPreferredColor( colorfld->color() );

    displayPage( sNamePage, false );
    setupgrp->setType( objid, sid );

    mpeserv->sendEvent( uiMPEPartServer::evStartSeedPick );
    stickSetChange(0);

    return true;
}


bool Wizard::leaveSeedSetupPage( bool process )
{
    mpeserv->sendEvent( uiMPEPartServer::evEndSeedPick );

    if ( !process ) return true;

    const int trackerid = mpeserv->getTrackerID( currentobject );
    const EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject(objid);
    const TypeSet<EM::PosID>* pids =
	emobj->getPosAttribList( EM::EMObject::sSeedNode );

    if ( !pids || !pids->size() )
    {
	mpeserv->sendEvent( uiMPEPartServer::evStartSeedPick );
	mErrRet( "You did not create any seedpoints" );
    }

    if ( !setupgrp->processInput() )
	mErrRet( "Please select Tracking Setup" );

    displayPage( sSeedSetupPage, false );
    adjustSeedBox();

    return true;
}


bool Wizard::prepareFinalizePage()
{
    typefld->setText(trackertype);
    return true;
}


bool Wizard::leaveFinalizePage(bool process)
{
    if ( !process ) return true;

    anotherSel(0);
    if ( anotherfld->getBoolValue() )
    {
	setTrackingType( typefld->text() );
    }

    return true;
}


void Wizard::isStarting()
{
    reloadattribdata = false;
    seedbox.setEmpty();
}



bool Wizard::isClosing(bool iscancel)
{
    if ( iscancel )
    {
	if ( trackercreated )
	{
	    //remove tracker
	}

	if ( objectcreated )
	{
	    //remove object
	}
    }

    if ( !seedbox.isEmpty() )
    {
	mpeserv->expandActiveArea(seedbox);
    }

    if ( reloadattribdata ) mpeserv->loadAttribData();

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
	case 1: return prepareSeedSetupPage();
	case 3: return prepareFinalizePage();
    }

    return true;
}


bool Wizard::leavePage( int page, bool process )
{
    switch ( page )
    {
	case 0: return leaveNamePage(process);
	case 1: return leaveSeedSetupPage(process);
	case 3: return leaveFinalizePage(process);
    }

    return true;
}


void Wizard::adjustSeedBox()
{
    const int trackerid = mpeserv->getTrackerID( currentobject );

    const EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject(objid);

    PtrMan<EM::EMObjectIterator> iterator = emobj->createIterator(sid);

    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	const Coord3 pos = emobj->getPos(pid);
	if ( !pos.isDefined() )
	    continue;

	const BinID bid = SI().transform(pos);
	if ( seedbox.isEmpty() )
	{
	    seedbox.hrg.start = seedbox.hrg.stop = bid;
	    seedbox.zrg.start = seedbox.zrg.stop = pos.z;
	}
	else
	{
	    seedbox.hrg.include(bid);
	    seedbox.zrg.include(pos.z);
	}
    }
}


void Wizard::reset()
{
    if ( objectcreated )
	defcolnr++;

    sid = -1;
    objectcreated = false;
    trackercreated = false;
    currentobject = -1;

    for ( int idx=0; idx<nrPages(); idx++ )
	displayPage( idx, true );
}


void Wizard::setTrackingType( const char* tp )
{
    trackertype = tp;
    updateDialogTitle();
}


void Wizard::setObject( const MultiID& mid, const EM::SectionID& sectionid )
{
    currentobject = mid;
    sid = sectionid;
    const EM::ObjectID objid = EM::EMM().multiID2ObjectID( mid );
    const EM::EMObject* emobj = EM::EMM().getObject( objid );
    if ( emobj )
	setTrackingType( emobj->getTypeStr() );
}


void Wizard::updateDialogTitle()
{
    BufferString str( trackertype ); str += " Tracking";
    setTitleText( str );
}


bool Wizard::createTracker()
{
    if ( currentobject==-1 )
    {
	EM::ObjectID objid = EM::EMM().createObject(trackertype,namefld->text());
	EM::EMObject* emobj = EM::EMM().getObject(objid);
	const int id = engine().addTracker( emobj );
	if ( id==-1 )
	{
	    pErrMsg( "Could not create tracker" );
	    return false;
	}

	EMTracker* tracker = engine().getTracker(id);

	if ( !engine().getEditor(objid,false) )
	    engine().getEditor(objid,true);

	mpeserv->activetrackerid = id;
	if ( !mpeserv->sendEvent( ::uiMPEPartServer::evAddTreeObject ) )
	{
	    pErrMsg("Could not add treeitem");
	    engine().removeTracker( id );
	    emobj->ref(); emobj->unRef();
	    return false;
	}

	currentobject = emobj->multiID();
	objectcreated = true;
	trackercreated = true;

	//PtrMan<Executor> saver = emobj->saver();
	//if ( saver ) saver->execute();
    }
    else if ( mpeserv->getTrackerID(currentobject)<0 )
    {
	const EM::ObjectID objid = EM::EMM().multiID2ObjectID( currentobject );
	EM::EMObject* emobj = EM::EMM().getObject( objid );

	if ( MPE::engine().addTracker(emobj)<0 )
	{
	    pErrMsg( "Could not create tracker" );
	    return false;
	}

	trackercreated = true;
    }

    return true;
}


void Wizard::setupChange( CallBacker* )
{
    const int trackerid = mpeserv->getTrackerID( currentobject );
    MPE::EMTracker* tracker = MPE::engine().getTracker( trackerid );
    MPE::EMSeedPicker* seedpicker = tracker->getSeedPicker(false);
    if ( !seedpicker )
	return;

    uiCursor::setOverride( uiCursor::Wait );
    seedpicker->reTrack();
    uiCursor::restoreOverride();
}

}; // namespace MPE
