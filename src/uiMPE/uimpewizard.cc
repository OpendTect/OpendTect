/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "uimpewizard.h"

#include "ctxtioobj.h"
#include "undo.h"
#include "emmanager.h"
#include "emhorizon.h"
#include "emseedpicker.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "faultseedpicker.h"
#include "horizon2dseedpicker.h"
#include "horizon3dseedpicker.h"
#include "ioman.h"
#include "ioobj.h"
#include "mpeengine.h"
#include "ptrman.h"
#include "randcolor.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "mousecursor.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimpepartserv.h"
#include "uimpe.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitextedit.h"

namespace MPE {

const int Wizard::sNamePage		= 0;
const int Wizard::sTrackModePage	= 1;
const int Wizard::sSeedSetupPage	= 2;
const int Wizard::sFinalizePage		= 3;

static const char* sSeedPickOptionsInfo()
{
    return
	"\nSeed Pick Options:\n"
	"*   left mouse-click on plane  =  pick seed & local track\n"
	"       + Ctrl-Shift  =  drop seed\n"
	"*   left mouse-click on seed  =  local retrack\n"
	"       + Ctrl  =  remove seed & local retrack\n"
	"       + Shift  =  remove seed & local erase\n"
	"       + Ctrl-Shift  =  undrop seed";
}

static const char* sTrackInVolInfo( const BufferString& trackertype )
{
    if ( trackertype == EMHorizon2DTranslatorGroup::keyword() ) return 
	"The 2D horizon is auto-tracked from seeds picked on 2D lines.\n\n"
	"Workflow:\n"
	"1) Define settings.\n"
	"2) Pick seeds on a 2D line (options see below).\n"
	"3) Finish wizard.\n"
	"4) Pick seeds on other 2D lines in your lineset.\n"
	"5) Use 'Derive 3D Horizon' to create a continuous horizon.\n";
   
    return
	"The horizon is (auto-) tracked inside a small track-volume.\n\n"
	"Workflow:\n"
	"1) Define settings\n"
	"2) Pick seed(s) on inline/crossline/trackplane (options see below)\n"
	"3) Finish wizard and position/resize track-volume\n"
	"4) Use toolbar to auto-track, plane-by-plane track, and edit\n"
	"5) Reposition track-volume and repeat step 4\n";
}


static const char* sLineTrackInfo( const BufferString& trackertype )
{
    if ( trackertype == EMHorizon2DTranslatorGroup::keyword() ) return 
	"The 2D horizon is auto-tracked between seeds picked on 2D lines.\n\n"
	"Workflow:\n"
	"1) Define settings.\n"
	"2) Pick seeds on a 2D line (options see below).\n"
	"3) Finish wizard.\n"
	"4) Pick seeds on other 2D lines in your lineset.\n"
	"5) Use 'Derive 3D Horizon' to create a continuous horizon.\n";

    return
	"The horizon is auto-tracked in the line direction only.\n\n"
	"Workflow:\n"
	"1) Define settings\n"
	"2) Pick seeds on inline/crossline/trackplane (options see below)\n"
	"3) Finish wizard\n"
	"4) Scroll line to new position or open new line, and pick new seeds\n"
	"5) Use 'Fill holes' to create continuous horizon\n";
}


static const char* sLineManualInfo( const BufferString& trackertype )
{
    if ( trackertype == EMHorizon2DTranslatorGroup::keyword() )
	return 
	"The 2D horizon is painted (linear interpolation) between seeds\n"
	"picked on 2D lines.\n\n"
	"Workflow:\n"
	"1) Finish wizard.\n"
	"2) Pick seeds on a 2D line (options see below)\n"
	"3) Pick seeds on other 2D lines in your lineset.\n"
	"4) Use 'Derive 3D Horizon' to create a continuous horizon.\n";


    return
	"The horizon is painted (linear interpolation) between picked seeds\n"
        "in the line direction only.\n\n"
	"Workflow:\n"
	"1) Finish wizard\n"
	"2) Pick seeds on inline/crossline/trackplane (options see below)\n"
	"3) Scroll line to new position or open new line, and pick new seeds\n"
	"4) Use 'Fill holes' to create continuous horizon\n";
}


static const IOObjContext* getContext( const char* type )
{
    if ( !strcmp(type,EMFaultStickSetTranslatorGroup::keyword() ) )
	return &EMFaultStickSetTranslatorGroup::ioContext();

    if ( !strcmp(type,EMFault3DTranslatorGroup::keyword() ) )
	return &EMFault3DTranslatorGroup::ioContext();

    if ( !strcmp(type,EMHorizon2DTranslatorGroup::keyword() ) )
	return &EMHorizon2DTranslatorGroup::ioContext();

    if ( !strcmp(type,EMHorizon3DTranslatorGroup::keyword() ) )
	return &EMHorizon3DTranslatorGroup::ioContext();

    return 0;
}


Wizard::Wizard( uiParent* p, uiMPEPartServer* mps )
    : uiWizard( p, uiDialog::Setup("Tracking Wizard",0,"108.0.0").modal(false) )
    , mpeserv(mps)
    , sid(-1)
    , currentobject(-1)
    , objectcreated(false)
    , trackercreated(false)
    , ioparentrycreated(false)
    , typefld(0)
    , anotherfld(0)
    , initialundoid_(mUdf(int))
    , oldsettingsseeds(0)
{
    objselgrp = createNamePage();
    addPage( objselgrp );
    addPage( createTrackModePage() );
    addPage( createSeedSetupPage() );
#ifdef __debug__
    //addPage( createFinalizePage() );
#endif
    setRotateMode( false );

    seedbox.setEmpty();
}


Wizard::~Wizard()
{
}


uiIOObjSelGrp* Wizard::createNamePage()
{
    // TODO: Make nice for Horizon(2D) & Fault (trackertype not yet set here!!)
    const IOObjContext* ctxttemplate = getContext( trackertype );
    if ( !ctxttemplate )
	ctxttemplate = &EMFault3DTranslatorGroup::ioContext();

    PtrMan<IOObjContext> ctxt = new IOObjContext( *ctxttemplate );
    ctxt->forread = false;

    const CtxtIOObj ctio( *ctxt );
    return new uiIOObjSelGrp( this, ctio );
}


#define mDefSeedConModeGrp( xmodegrp, typ ) \
    xmodegrp = new uiButtonGroup( grp, "Mode" ); \
    xmodegrp->setExclusive( true ); \
    for ( int idx=0; idx<typ##SeedPicker::nrSeedConnectModes(); idx++ ) \
    { \
	uiRadioButton* butptr = new uiRadioButton( xmodegrp, \
			    typ##SeedPicker::seedConModeText(idx,false) ); \
	butptr->activated.notify( mCB(this,Wizard,seedModeChange) ); \
    } \
    xmodegrp->selectButton( typ##SeedPicker::defaultSeedConMode() ); \
    xmodegrp->attach( alignedAbove, colorfld );

uiGroup* Wizard::createTrackModePage()
{
    uiGroup* grp = new uiGroup( this, "Page 2" );

    colorfld = new uiColorInput( grp,
			         uiColorInput::Setup(getRandStdDrawColor() ).
				 lbltxt("Object color") );
    colorfld->colorChanged.notify( mCB(this,Wizard,colorChangeCB) );

    mDefSeedConModeGrp( hmodegrp, Horizon3D ); 
    mDefSeedConModeGrp( h2dmodegrp, Horizon2D ); 
//  mDefSeedConModeGrp( fmodegrp, Fault ); 

    uiSeparator* sep = new uiSeparator( grp );
    sep->attach( stretchedBelow, colorfld, -2 );

    uiLabel* infolbl = new uiLabel( grp, "Info:" );
    infolbl->attach( alignedBelow, sep );

    infofld = new uiTextEdit( grp, "Info", true );
    infofld->setPrefHeightInChar( 12 );
    infofld->setPrefWidthInChar( 80 );
    infofld->attach( alignedBelow, infolbl );

    return grp;
}


#define mDefSetupGrp( xsetupgrp, typ ) \
    xsetupgrp = uiMPE().setupgrpfact.create( \
	    			grp, EM##typ##TranslatorGroup::keyword(), 0 ); \
    xsetupgrp->attach( centeredAbove, lbl );

uiGroup* Wizard::createSeedSetupPage()
{
    uiGroup* grp = new uiGroup( this, "Page 3" );
    uiLabel* lbl = new uiLabel( grp,
			    "Evaluate settings by picking one or more seeds" );

    mDefSetupGrp( hsetupgrp, Horizon3D );
    mDefSetupGrp( h2dsetupgrp, Horizon2D );
//  mDefSetupGrp( fsetupgrp, Fault );

    uiPushButton* applybut = new uiPushButton( grp, "Retrack all", true );
    applybut->activated.notify( mCB(this,Wizard,retrackCB) );
    applybut->attach( centeredBelow, lbl );

    return grp;
}


uiGroup* Wizard::createFinalizePage()
{
    uiGroup* grp = new uiGroup( this, "Page 4" );
    BufferStringSet trackernames;
    engine().getAvailableTrackerTypes( trackernames );

    BufferString str("Do you want to track another ");
    str += trackernames.size()>1 ? "surface" : (const char*) trackernames[0];
    str += "?";
    uiLabel* lbl = new uiLabel( grp, str );

    anotherfld = new uiGenInput( grp, "", BoolInpSpec(true) );
    anotherfld->attach( alignedBelow, lbl );
    anotherfld->valuechanged.notify( mCB(this,Wizard,anotherSel) );

    if ( trackernames.size()>1 )
    {
	typefld = new uiGenInput( grp, "Type",
				  StringListInpSpec(trackernames) );
	typefld->attach( alignedBelow, anotherfld );
    }

//  anotherSel(0);
    return grp;
}


bool Wizard::prepareNamePage()
{
    const IOObjContext* ctxttemplate = getContext(trackertype);
    if ( !ctxttemplate )
    {
	pErrMsg("Cannot find context");
	return false;
    }

    PtrMan<IOObjContext> ctxt = new IOObjContext(*ctxttemplate);
    ctxt->forread = false;

    objselgrp->setContext( *ctxt );

    if ( objselgrp->getListField()->size() )
	objselgrp->getListField()->selectAll(false);

    objselgrp->getNameField()->setFocus();

    return true;
}


bool Wizard::leaveNamePage( bool process )
{
    if ( !process ) return true;

    const char* newobjnm = objselgrp->getNameField()->text();
    PtrMan<IOObj> ioobj = IOM().getLocal( newobjnm );
    if ( ioobj )
    {
	const bool askoverwrite =
	    objselgrp->getCtxtIOObj().ctxt.validIOObj( *ioobj );
	if ( !askoverwrite )
	{
	    uiMSG().error( "An object of different type has the same name.\n"
		    	   "Please give another name." );
	    return false;
	}
	else if ( !uiMSG().askOverwrite("An object with this name exists. "
		    		   "Overwrite?") )
	    return false;
    }

    objselgrp->setConfirmOverwrite( false );
    if ( !objselgrp->processInput() )
    {
	pErrMsg("Could not process input");
	return false;
    }

    ioparentrycreated = !ioobj;
    const int nrsel = objselgrp->nrSel();
    PtrMan<IOObj> newioobj = nrsel>0 ? IOM().get(objselgrp->selected(0)) : 0;
    if ( !newioobj )
    {
	pErrMsg( "Could not get ioobj" );
	return false;
    }

    const bool isimpl = newioobj->implExists( false );
    const bool isreadonly = isimpl && newioobj->implReadOnly();

    const EM::ObjectID objid = EM::EMM().getObjectID( newioobj->key() );
    const EM::EMObject* emobj = EM::EMM().getObject( objid );
    if ( emobj )
    {
	uiMSG().error("An object with this name exists and is currently\n"
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
    else
	currentobject = -1;

    return true;
}


#define mSelectSeedConModeGrp( xmodegrp, typ ) \
    xmodegrp->display( false, true ); \
    if ( trackertype == EM##typ##TranslatorGroup::keyword() && \
	 typ##SeedPicker::nrSeedConnectModes()>0 ) \
	modegrp = xmodegrp; \
    xmodegrp->display( xmodegrp==modegrp, true );
    
bool Wizard::prepareTrackModePage()
{
    if ( currentobject!=-1 )
    {
	EM::EMObject* emobj = EM::EMM().getObject( currentobject );
	colorfld->setColor( emobj->preferredColor() );
    }

    if ( !createTracker() )
	return false;

    const int trackerid = mpeserv->getTrackerID( currentobject );
    EMTracker* tracker = MPE::engine().getTracker( trackerid );
    const EM::ObjectID objid = tracker->objectID();
    EM::EMObject* emobj = EM::EMM().getObject(objid);
    if ( sid==-1 )
        sid = emobj->sectionID( emobj->nrSections()-1 );

    mSelectSeedConModeGrp( hmodegrp, Horizon3D );
    mSelectSeedConModeGrp( h2dmodegrp, Horizon2D );
//  mSelectSeedConModeGrp( fmodegrp, Fault );

    seedModeChange(0);
    colorChangeCB(0);
    return true;
}


bool Wizard::leaveTrackModePage( bool process )
{
    if ( !process ) restoreObject();
	
    if ( currentPageIdx()==lastPage() )
	return finalizeCycle();

    return true;
}


#define mSelectSetupGrp( xsetupgrp, typ ) \
    xsetupgrp->display( false, true ); \
    if ( trackertype == EM##typ##TranslatorGroup::keyword() ) \
	setupgrp = xsetupgrp; \
    xsetupgrp->display( xsetupgrp==setupgrp, true );

bool Wizard::prepareSeedSetupPage()
{
    mSelectSetupGrp( hsetupgrp, Horizon3D );
    mSelectSetupGrp( h2dsetupgrp, Horizon2D );
    //mSelectSetupGrp( fsetupgrp, Fault );

    const int trackerid = mpeserv->getTrackerID( currentobject );
    EMTracker* tracker = MPE::engine().getTracker( trackerid );
    SectionTracker* sectiontracker = tracker->getSectionTracker( sid, true );
    if ( !sectiontracker ) return false;
    setupgrp->setSectionTracker( sectiontracker );
    setupgrp->setAttribSet( mpeserv->getCurAttrDescSet(tracker->is2D()) ); 

    mpeserv->sendEvent( uiMPEPartServer::evStartSeedPick() );
    EMSeedPicker* seedpicker = tracker->getSeedPicker( true );

    if ( currentPageIdx()==lastPage() )
	setRotateMode(false);

    NotifierAccess* addrmseednotifier = seedpicker->aboutToAddRmSeedNotifier();
    if ( addrmseednotifier ) 
	addrmseednotifier->notify( mCB(this,Wizard,aboutToAddRemoveSeed) );
    NotifierAccess* surfchangenotifier = seedpicker->madeSurfChangeNotifier();
    if ( surfchangenotifier ) 
	surfchangenotifier->notify( mCB(this,Wizard,updateFinishButton) );
    updateFinishButton(0);
    initialundoid_ = EM::EMM().undo().currentEventID();

    return true;
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

bool Wizard::leaveSeedSetupPage( bool process )
{
    mGetSeedPicker(false);

    if ( process && !setupgrp->commitToTracker() )
	return false;

    NotifierAccess* addrmseednotifier = seedpicker->aboutToAddRmSeedNotifier();
    if ( addrmseednotifier ) 
	addrmseednotifier->remove( mCB(this,Wizard,aboutToAddRemoveSeed) );
    NotifierAccess* surfchangenotifier = seedpicker->madeSurfChangeNotifier();
    if ( surfchangenotifier ) 
	surfchangenotifier->remove( mCB(this,Wizard,updateFinishButton) );

    setButtonSensitive( uiDialog::CANCEL, true );
    if ( !process )
    {
	mpeserv->sendEvent( uiMPEPartServer::evEndSeedPick() );
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


bool Wizard::leaveFinalizePage( bool process )
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
	adjustSeedBox();
	EM::EMObject* emobj = EM::EMM().getObject( currentobject );
	PtrMan<Executor> saver = emobj->saver();
	if ( saver ) saver->execute();
    }

    return true;
}


void Wizard::isStarting()
{
    seedbox.setEmpty();
    colorfld->setColor( getRandStdDrawColor() );
}


void Wizard::restoreObject()
{
    if ( !mIsUdf(initialundoid_) )
    {
	EM::EMObject* emobj = EM::EMM().getObject( currentobject );
	emobj->setBurstAlert( true );
	
	EM::EMM().undo().unDo(
	                EM::EMM().undo().currentEventID()-initialundoid_);
	EM::EMM().undo().removeAllAfterCurrentEvent();

	emobj->setBurstAlert( false );
    }

    if ( ioparentrycreated )
    {
	const MultiID mid = EM::EMM().getMultiID(currentobject);
	PtrMan<IOObj> ioobj = IOM().get(mid);
	
	if ( ioobj )
	{
	    if ( !fullImplRemove(*ioobj) || !IOM().permRemove(mid) )
		pErrMsg( "Could not remove object" );
	}
    }

    NotifyStopper notifystopper( MPE::engine().trackeraddremove );

    // This must come before tracker is removed since
    // applman needs tracker to know what to remove.
    // And after io-stuff removal which needs valid MultiID 
    if ( objectcreated )
    {
	mpeserv->sendEvent( ::uiMPEPartServer::evRemoveTreeObject() );
    }

    if ( trackercreated )
    {
	const int trackerid = mpeserv->getTrackerID( currentobject );
	MPE::engine().removeTracker( trackerid );
    }

    trackercreated = false;
    objectcreated = false;
    currentobject = -1;
    initialundoid_ = mUdf(int);
    sid = -1;
}


bool Wizard::isClosing( bool iscancel )
{
    mpeserv->blockDataLoading( true );
    mpeserv->sendEvent( uiMPEPartServer::evEndSeedPick() );
    mpeserv->blockDataLoading( false );
    if ( iscancel )
    {
	restoreObject();
	MPE::engine().trackeraddremove.trigger();
    }
    else 
    {
	mGetSeedPicker(false);
	mpeserv->blockDataLoading( true );
	if ( seedpicker->doesModeUseVolume() && !seedbox.isEmpty() )
	    mpeserv->expandActiveVolume(seedbox);

	if ( !seedpicker->doesModeUseVolume() )
	    mpeserv->sendEvent( uiMPEPartServer::evStartSeedPick() );
	
	mpeserv->blockDataLoading( false );
	mpeserv->postponeLoadingCurVol();
	mpeserv->sendEvent( uiMPEPartServer::evMPEDispIntro() );
	mpeserv->sendEvent( uiMPEPartServer::evShowToolbar() );
	if ( seedpicker->doesModeUseSetup() )
	    mpeserv->saveSetup( EM::EMM().getMultiID(currentobject) );
    }
    mpeserv->sendEvent( ::uiMPEPartServer::evSetupClosed() );
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
	mpeserv->sendEvent( uiMPEPartServer::evUpdateTrees() );
    }
}


#define mNamePage	0
#define mModePage	1
#define mSeedPage	2
#define mFinalizePage	3


bool Wizard::preparePage( int page )
{
    switch ( page )
    {
	case mNamePage:		return prepareNamePage();
	case mModePage:		return prepareTrackModePage();
	case mSeedPage:		return prepareSeedSetupPage();
	case mFinalizePage:	return prepareFinalizePage();
    }

    return true;
}


bool Wizard::leavePage( int page, bool process )
{
    switch ( page )
    {
	case mNamePage:		return leaveNamePage( process);
	case mModePage:		return leaveTrackModePage( process );
	case mSeedPage:		return leaveSeedSetupPage( process );
	case mFinalizePage:	return leaveFinalizePage( process );
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
	     !emobj->isPosAttrib( pid, EM::EMObject::sSeedNode() ) )
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
    sid = -1;
    objectcreated = false;
    ioparentrycreated = false;
    trackercreated = false;
    currentobject = -1;
    initialundoid_ = mUdf(int);

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
    // Must reserve text space for longest type while initializing the wizard.
    // Must pad with double spaces to be safe in case of variable width fonts.

    const BufferString current( trackertype );
    const BufferString longest( EMHorizon2DTranslatorGroup::keyword() );
    const int spacestoadd = longest.size() - current.size();

    BufferString str;
    for ( int idx=0; idx<spacestoadd; idx++ )
	str += " ";
    str += current; str += " Tracking";
    for ( int idx=0; idx<spacestoadd; idx++ )
	str += " ";
    
    setTitleText( str );
}


bool Wizard::createTracker()
{
    NotifyStopper notifystopper( MPE::engine().trackeraddremove );

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

	mpeserv->activetrackerid_ = id;
	if ( !mpeserv->sendEvent( ::uiMPEPartServer::evAddTreeObject() ) )
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
    const int newmode = modegrp ? modegrp->selectedId() : -1;
    BufferString infotext;

    if ( newmode == 0 )
	infotext += sTrackInVolInfo(trackertype);
    else if ( newmode == 1 )
	infotext += sLineTrackInfo(trackertype);
    else
	 infotext += sLineManualInfo(trackertype);

    infotext += sSeedPickOptionsInfo();
    infofld->setText( infotext );
    mGetSeedPicker();
    seedpicker->setSeedConnectMode( newmode );
    mpeserv->sendEvent( uiMPEPartServer::evUpdateSeedConMode() );

    displayPage( sSeedSetupPage, seedpicker->doesModeUseSetup() );
}


void Wizard::retrackCB( CallBacker* )
{
    bool fieldchange;
    if ( !setupgrp->commitToTracker(fieldchange) )
	return;

    mGetSeedPicker();

    const CubeSampling curvol = engine().activeVolume();
    if ( curvol.nrInl()==1 || curvol.nrCrl()==1 )
	mpeserv->loadAttribData();

    Undo& undo = EM::EMM().undo();
    int cureventnr = undo.currentEventID();
    undo.setUserInteractionEnd( cureventnr, false );

    EM::EMObject* emobj = EM::EMM().getObject( currentobject );
    emobj->setBurstAlert( true );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    seedpicker->reTrack();
    MouseCursorManager::restoreOverride();
    emobj->setBurstAlert( false );

    undo.setUserInteractionEnd( undo.currentEventID() );
}


void Wizard::aboutToAddRemoveSeed( CallBacker* )
{
    mGetSeedPicker();
    bool fieldchange;
    const bool isvalidsetup = setupgrp->commitToTracker(fieldchange);
    seedpicker->blockSeedPick( !isvalidsetup );
	
    if ( isvalidsetup && fieldchange )
    {
	mpeserv->loadAttribData();
	oldsettingsseeds = seedpicker->nrSeeds();
    }
}


void Wizard::updateFinishButton( CallBacker* )
{
    mGetSeedPicker();
    const int nrseeds = seedpicker->nrSeeds();

    const bool finishenabled = nrseeds >= seedpicker->minSeedsToLeaveInitStage();
    setButtonSensitive( uiDialog::CANCEL, finishenabled );

    if ( oldsettingsseeds && oldsettingsseeds+nrseeds>2 )
    {
	uiMSG().message( "Click 'Retrack all' to view the effect of your\n"
			 "new settings around the other seeds as well." );
    }
    oldsettingsseeds = 0;
}


}; // namespace MPE
