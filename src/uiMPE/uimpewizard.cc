/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uimpewizard.cc,v 1.44 2006-04-07 15:17:50 cvsjaap Exp $
________________________________________________________________________

-*/


#include "uimpewizard.h"

#include "ctxtioobj.h"
#include "emhistory.h"
#include "emmanager.h"
#include "emobject.h"
#include "emseedpicker.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "faultseedpicker.h"
#include "horizonseedpicker.h"
#include "ioman.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "ptrman.h"
#include "survinfo.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uicursor.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimpepartserv.h"
#include "uimpesetup.h"
#include "uiseparator.h"
#include "uispinbox.h"

namespace MPE {

int Wizard::defcolnr = 0;

const int Wizard::sNamePage		= 0;
const int Wizard::sSeedSetupPage	= 1;
const int Wizard::sFinalizePage		= 2;


Wizard::Wizard( uiParent* p, uiMPEPartServer* mps )
    : uiWizard( p, uiDialog::Setup( "Tracking Wizard", "XXXXXXX Tracking", 
				    "" ).modal(false) )
    , mpeserv(mps)
    , sid(-1)
    , currentobject(-1)
    , objectcreated(false)
    , trackercreated(false)
    , ioparentrycreated(false)
    , typefld(0)
    , anotherfld(0)
{
    objselgrp = createNamePage();
    addPage( objselgrp );
    addPage( createSeedSetupPage() );
#ifdef __debug__
    //addPage( createFinalizePage() );
#endif

    seedbox.setEmpty();
    setHelpID( "108.0.0" );
}


Wizard::~Wizard()
{
}


uiIOObjSelGrp* Wizard::createNamePage()
{
    const IOObjContext* ctxttemplate = EM::EMM().getContext(trackertype);
    if ( !ctxttemplate )
    {
	ctxttemplate = EM::EMM().getContext("Fault");
    }

    PtrMan<IOObjContext> ctxt = new IOObjContext(*ctxttemplate);
    ctxt->forread = false;

    const CtxtIOObj ctio( *ctxt );
    return new uiIOObjSelGrp( this, ctio );
}


const char* Wizard::seedPickText( bool doneedseed ) const
{
    if ( doneedseed )
	return ( "Must pick seedpoint(s) by clicking on a slice.\n"
		 "Remove seedpoints by ctrl-click on them,\n" 
		 "or use shift-click instead to omit retracking." );
    else
	return ( "May pick seedpoint(s) by clicking on a slice.\n"
		 "Remove seedpoints by ctrl-click on them,\n" 
		 "or use shift-click instead to omit retracking." );
}


#define mDefSeedConModeGrp( xmodegrp, typ ) \
    xmodegrp = new uiButtonGroup( grp, "Mode" ); \
    xmodegrp->setRadioButtonExclusive( true ); \
    for ( int idx=0; idx<typ##SeedPicker::nrSeedConnectModes(); idx++ ) \
    { \
	uiRadioButton* butptr = new uiRadioButton( hmodegrp, \
			    typ##SeedPicker::seedConModeText(idx,false) ); \
	butptr->activated.notify( mCB(this, Wizard, seedModeChange) ); \
    } \
    xmodegrp->selectButton( typ##SeedPicker::defaultSeedConMode() ); \
    xmodegrp->attach( alignedAbove, setupgrp ); \
    modesep->attach( stretchedBelow, xmodegrp ); 

uiGroup* Wizard::createSeedSetupPage()
{
    uiGroup* grp = new uiGroup( this, "Page 2" );

    setupgrp = new uiSetupSel( grp, mpeserv->attrset );
    setupgrp->setupchg.notify( mCB(this,Wizard,setupChange) );
    
    uiSeparator* sep1 = new uiSeparator( grp, "Separator 1" );
    sep1->attach( stretchedBelow, setupgrp );
    
    picktxt = new uiLabel( grp, seedPickText(true) );
    picktxt->attach( ensureBelow, sep1 );
    picktxt->attach( alignedBelow, setupgrp );
    
    colorfld = new uiColorInput( grp, Color::drawDef(defcolnr++), 
				 "Object color" );
    colorfld->colorchanged.notify( mCB(this,Wizard,colorChangeCB) );
    colorfld->attach( ensureBelow, picktxt );
    colorfld->attach( alignedBelow, picktxt );

    modesep = new uiSeparator( grp, "Separator 0" );
    modesep->attach( stretchedAbove, setupgrp );
    
    mDefSeedConModeGrp( hmodegrp, Horizon ); 
    mDefSeedConModeGrp( fmodegrp, Fault ); 

    return grp;
}


uiGroup* Wizard::createFinalizePage()
{
    uiGroup* grp = new uiGroup( this, "Page 4" );
    BufferStringSet trackernames;
    engine().getAvaliableTrackerTypes( trackernames );

    BufferString str("Do you want to track another ");
    str += trackernames.size()>1 ? "surface" : (const char*) trackernames[0];
    str += "?";
    uiLabel* lbl = new uiLabel( grp, str );

    anotherfld = new uiGenInput( grp, "", BoolInpSpec() );
    anotherfld->attach( alignedBelow, lbl );
    anotherfld->valuechanged.notify( mCB(this,Wizard,anotherSel) );

    if ( trackernames.size()>1 )
    {
	typefld = new uiGenInput( grp, "Type",
				  StringListInpSpec(trackernames) );
	typefld->attach( alignedBelow, anotherfld );
    }

    anotherSel(0);
    return grp;
}


bool Wizard::prepareNamePage()
{
    const IOObjContext* ctxttemplate = EM::EMM().getContext(trackertype);
    if ( !ctxttemplate )
    {
	pErrMsg("Cannot find context");
	return false;
    }

    PtrMan<IOObjContext> ctxt = new IOObjContext(*ctxttemplate);
    ctxt->forread = false;

    const CtxtIOObj ctio( *ctxt );
    objselgrp->setContext( ctio );

    if ( objselgrp->getListField()->box()->size() )
	objselgrp->getListField()->box()->selectAll(false);

    objselgrp->getNameField()->setFocus();

    return true;
}

#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool Wizard::leaveNamePage( bool process )
{
    if ( !process ) return true;

    bool didexist = true;
    const char* newobjnm = objselgrp->getNameField()->text(); 
    if ( *newobjnm )
    {
	PtrMan<IOObj> ioobj = IOM().getLocal( newobjnm );
	if ( !ioobj ) didexist = false;
	
    }

    if ( !objselgrp->processInput() )
    {
	pErrMsg("Could not process input");
	return false;
    }

    ioparentrycreated = !didexist;

    const IOObj* ioobj = objselgrp->selected(0);
    if ( !ioobj )
    {
	pErrMsg( "Could not get ioobj");
	return false;
    }

    const bool isimpl = ioobj->implExists(false);
    const bool isreadonly = isimpl && ioobj->implReadOnly();

    EM::ObjectID objid = EM::EMM().getObjectID( ioobj->key() );
    EM::EMObject* emobj = EM::EMM().getObject( objid );
    if ( emobj )
    {
	uiMSG().error("An object with this name exist and is currently\n"
		      "loaded. Please select another name or quit the\n"
		      "wizard and remove the object with this name from\n"
		      "the tree.");
	return false;
    }
    else if ( isreadonly )
    {
	uiMSG().error("This object is marked as read-only. Please select\n"
		      "another object or make it writable." );
	return false;
    }
    else if ( didexist )
    {
	if ( !uiMSG().askGoOn("An object with that name does already exist."
			      " Overwrite?",true) )
	    return false;
    }
    else
	currentobject = -1;

    return true;
}


#define mSelectSeedConModeGrp( xmodegrp, typ ) \
    xmodegrp->display( false, true ); \
    if ( trackertype == EM##typ##TranslatorGroup::keyword && \
	 typ##SeedPicker::nrSeedConnectModes()>0 ) \
	modegrp = xmodegrp; \
    xmodegrp->display( xmodegrp==modegrp, true );
    
bool Wizard::prepareSeedSetupPage()
{
    if ( currentobject!=-1 )
    {
	EM::EMObject* emobj = EM::EMM().getObject( currentobject );
	colorfld->setColor( emobj->preferredColor() );
    }
    else
    {
	colorfld->setColor( Color::drawDef(defcolnr) );
    }

    if ( !createTracker() )
	return false;

    const int trackerid = mpeserv->getTrackerID( currentobject );

    EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject(objid);
    if ( sid==-1 )
        sid = emobj->sectionID( emobj->nrSections()-1 );

    setupgrp->setType( objid, sid );

    modegrp=0;
    mSelectSeedConModeGrp( hmodegrp, Horizon );
    mSelectSeedConModeGrp( fmodegrp, Fault );
    modesep->display( modegrp, true );	

    seedModeChange(0);
    colorChangeCB(0);

    mpeserv->sendEvent( uiMPEPartServer::evStartSeedPick );
    EMSeedPicker* seedpicker = tracker->getSeedPicker( true );

    if ( currentPageIdx()==lastPage() )
	setRotateMode(false);

    emobj->notifier.notify( mCB(this,Wizard,updateFinishButton) );
    updateFinishButton(0);
    initialhistorynr = EM::EMM().history().currentEventNr();
    return true;
}


bool Wizard::leaveSeedSetupPage( bool process )
{
    mpeserv->blockdataloading = true;
    mpeserv->sendEvent( uiMPEPartServer::evEndSeedPick );
    mpeserv->blockdataloading = false;

    EM::EMObject* emobj = EM::EMM().getObject(currentobject);
    emobj->notifier.remove( mCB(this,Wizard,updateFinishButton) );
    setButtonSensitive( uiDialog::CANCEL, true );
    if ( !process )
    {
	restoreObject();
	return true;
    }

    if ( currentPageIdx()==lastPage() )
	return finalizeCycle();

    return true;
}


bool Wizard::prepareFinalizePage()
{
    if ( typefld ) typefld->setText(trackertype);
    return true;
}


bool Wizard::leaveFinalizePage(bool process)
{
    if ( !process ) return true;

    anotherSel(0);
    if ( anotherfld->getBoolValue() )
	setTrackingType( typefld ? typefld->text() : (const char*)trackertype );

    return finalizeCycle();
}


bool Wizard::finalizeCycle()
{
    if ( objectcreated )
    {
	EM::EMObject* emobj = EM::EMM().getObject( currentobject );
	PtrMan<Executor> saver = emobj->saver();

	if ( saver ) saver->execute();
	adjustSeedBox();
    }

    return true;
}


void Wizard::isStarting()
{
    seedbox.setEmpty();
}


void Wizard::restoreObject()
{
    if  ( !mIsUdf(initialhistorynr) )
    {
	EM::EMM().history().unDo(
	                EM::EMM().history().currentEventNr()-initialhistorynr);
	EM::EMM().history().setCurrentEventAsLast();
    }

    //This must come before tracker is removed since
    //applman needs tracker to know what to remove.
    if ( objectcreated )
    {
	mpeserv->sendEvent( ::uiMPEPartServer::evRemoveTreeObject );
    }

    if ( ioparentrycreated )
    {
	const MultiID mid = EM::EMM().getMultiID(currentobject);
	PtrMan<IOObj> ioobj = IOM().get(mid);

	if ( !ioobj || !fullImplRemove(*ioobj) ||
	     !IOM().permRemove(mid) )
	{
	    pErrMsg( "Could not remove object" );
	}

    }

    if ( trackercreated )
    {
	const int trackerid = mpeserv->getTrackerID( currentobject );
	MPE::engine().removeTracker( trackerid );
    }

    ioparentrycreated = false;
    trackercreated = false;
    objectcreated = false;
    currentobject = -1;
    initialhistorynr = mUdf(int);
    sid = -1;
}


#define mGetSeedPicker( retfld ) \
    const int trackerid = mpeserv->getTrackerID( currentobject ); \
    if ( trackerid == -1 ) \
	return retfld; \
    EMTracker* tracker = engine().getTracker( trackerid ); \
    if ( !tracker ) \
	return retfld; \
    EMSeedPicker* seedpicker = tracker->getSeedPicker( true ); \
    if ( !seedpicker ) \
	return retfld; 

bool Wizard::isClosing( bool iscancel )
{
    if ( iscancel )
	restoreObject();
    else 
    {
	mGetSeedPicker(false);
	if ( seedpicker->doesModeUseVolume() && !seedbox.isEmpty() )
	    mpeserv->expandActiveVolume(seedbox);
        mpeserv->sendEvent( uiMPEPartServer::evShowToolbar );
    }
    mpeserv->sendEvent( ::uiMPEPartServer::evWizardClosed );
    return true;
}


void Wizard::anotherSel( CallBacker* )
{
    const bool cont = anotherfld->getBoolValue();
    typefld->display( cont );
    setRotateMode( cont );
}


void Wizard::colorChangeCB( CallBacker* )
{
    if ( currentobject!=-1 )
    {
	EM::EMObject* emobj = EM::EMM().getObject( currentobject );
	emobj->setPreferredColor( colorfld->color() );
    }
}


#define mNamePage	0
#define mSeedPage	1
#define mFinalizePage	2


bool Wizard::preparePage( int page )
{
    switch ( page )
    {
	case mNamePage:		return prepareNamePage();
	case mSeedPage:		return prepareSeedSetupPage();
	case mFinalizePage:	return prepareFinalizePage();
    }

    return true;
}


bool Wizard::leavePage( int page, bool process )
{
    switch ( page )
    {
	case mNamePage:		return leaveNamePage(process);
	case mSeedPage:		return leaveSeedSetupPage(process);
	case mFinalizePage:	return leaveFinalizePage(process);
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
	if ( !pos.isDefined() || 
	     !emobj->isPosAttrib( pid, EM::EMObject::sSeedNode ) )
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
    ioparentrycreated = false;
    trackercreated = false;
    currentobject = -1;
    initialhistorynr = mUdf(int);

    for ( int idx=0; idx<nrPages(); idx++ )
	displayPage( idx, true );
}


void Wizard::setTrackingType( const char* tp )
{
    trackertype = tp;
    updateDialogTitle();
}


void Wizard::setObject( const EM::ObjectID& id, const EM::SectionID& sectionid )
{
    currentobject = id;
    sid = sectionid;
    const EM::EMObject* emobj = EM::EMM().getObject( id );
    if ( emobj ) setTrackingType( emobj->getTypeStr() );
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
	const char* nm = objselgrp->getNameField()->text();
	EM::ObjectID objid = EM::EMM().createObject( trackertype, nm );
	EM::EMObject* emobj = EM::EMM().getObject( objid );
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

	currentobject = objid;
	objectcreated = true;
	trackercreated = true;
    }
    else if ( mpeserv->getTrackerID(currentobject)<0 )
    {
	EM::EMObject* emobj = EM::EMM().getObject( currentobject );

	if ( MPE::engine().addTracker(emobj)<0 )
	{
	    pErrMsg( "Could not create tracker" );
	    return false;
	}

	trackercreated = true;
    }

    return true;
}


void Wizard::seedModeChange( CallBacker* )
{
    mGetSeedPicker();
    const int newmode = modegrp ? modegrp->selectedId() : -1;
    seedpicker->setSeedConnectMode( newmode );

    const bool needsetup = seedpicker->doesModeUseSetup();
    setupgrp->setSensitive( needsetup );

    const bool newmodeneedseed = seedpicker->isMinimumNrOfSeeds() > 0; 
    picktxt->setText( seedPickText(newmodeneedseed) );

    setupChange(0);
}


void Wizard::setupChange( CallBacker* )
{
    mGetSeedPicker();

    const bool allowpicking = !setupgrp->sensitive() ||
			      setupgrp->isSetToValidSetup(); 
    picktxt->setSensitive( allowpicking );
    colorfld->setSensitive( allowpicking );
    seedpicker->blockSeedPick( !allowpicking );

    updateFinishButton(0);

    uiCursor::setOverride( uiCursor::Wait );
    seedpicker->reTrack();
    uiCursor::restoreOverride();
}


void Wizard::updateFinishButton( CallBacker* )
{
    mGetSeedPicker();
    const bool finishenabled = !seedpicker->isSeedPickBlocked() 
		   && seedpicker->nrSeeds() >= seedpicker->isMinimumNrOfSeeds();
    setButtonSensitive( uiDialog::CANCEL, finishenabled );
}


}; // namespace MPE
