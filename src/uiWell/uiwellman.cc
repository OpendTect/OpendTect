/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellman.h"

#include "bufstringset.h"
#include "ioman.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "ptrman.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "welltrack.h"
#include "welltransl.h"
#include "wellwriter.h"

#include "uigeninputdlg.h"
#include "uigroup.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uimnemonicsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitoolbutton.h"
#include "uiwelldisplayserver.h"
#include "uiwelldlgs.h"
#include "uiwelllogcalc.h"
#include "uiwelllogimpexp.h"
#include "uiwelllogtools.h"
#include "uiwellmarkerdlg.h"

mDefineInstanceCreatedNotifierAccess(uiWellMan)


uiWellMan::uiWellMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(uiStrings::phrManage(uiStrings::sWell(
								      mPlural)),
				     mODHelpKey(mWellManHelpID))
				.nrstatusflds(1).modal(false),
		   WellTranslatorGroup::ioContext(),
		   WellTranslatorGroup::sGroupName())
{
    createDefaultUI();

    addManipButton( "copyobj", tr("Copy Well"), mCB(this,uiWellMan,copyPush) );

    if ( SI().zIsTime() )
    {
	addManipButton( "z2t",
			tr("Set Depth to Time Model from other Well"),
			mCB(this,uiWellMan,bulkD2TCB) );
    }

    logsgrp_ = new uiGroup( listgrp_, "Logs group" );
    uiListBox::Setup su( OD::ChooseAtLeastOne, uiStrings::sLogs(),
			 uiListBox::AboveMid );
    logsfld_ = new uiListBox( logsgrp_, su, "lognames" );
    logsfld_->setHSzPol( uiObject::Wide );

    auto* logsbgrp = new uiButtonGroup( listgrp_, "Logs buttons",
					OD::Horizontal );
    addlogsbut_ = new uiPushButton( logsbgrp, uiStrings::sImport(), false );
    addlogsbut_->activated.notify( mCB(this,uiWellMan,importLogs) );
    calclogsbut_ = new uiPushButton( logsbgrp, uiStrings::sCreate(), false);
    calclogsbut_->activated.notify( mCB(this,uiWellMan,calcLogs) );
    calclogsbut_->attach( rightOf, addlogsbut_ );

    logsbgrp->attach( centeredBelow, logsgrp_ );

    auto* butgrp = new uiManipButGrp( logsfld_ );
    logvwbut_ = butgrp->addButton( "view_log", mJoinUiStrs(sView(),
					      sSelectedLog().toLower()),
					      mCB(this,uiWellMan,viewLogPush) );
    logrenamebut_ = butgrp->addButton( uiManipButGrp::Rename,
		      uiStrings::phrRename(uiStrings::sSelectedLog().toLower()),
		      mCB(this,uiWellMan,renameLogPush) );
    logrmbut_ = butgrp->addButton( uiManipButGrp::Remove,
		uiStrings::phrRemove(uiStrings::sSelectedLog(mPlural).toLower())
		, mCB(this,uiWellMan,removeLogPush) );
    logcopybut_ = butgrp->addButton( "copyobj",
			uiStrings::phrCopy( uiStrings::sWellLog(mPlural) ),
			mCB(this,uiWellMan,copyLogPush) );
    logexpbut_ = butgrp->addButton( "export",
			uiStrings::phrExport( uiStrings::sWellLog(mPlural) ),
			mCB(this,uiWellMan,exportLogs) );
    loguombut_ = butgrp->addButton( "unitsofmeasure",
					tr("View/edit unit of measure"),
					mCB(this,uiWellMan,logUOMPush) );
    logmnembut_= butgrp->addButton( "mnemonics",
					tr("View/edit mnemonic"),
					mCB(this,uiWellMan,logMnemPush) );
    defmnemlogbut_ = butgrp->addButton( "defmnemlog",
					tr("Set/edit Default log for Mnemonic"),
					mCB(this,uiWellMan,defMnemLogPush) );
    custommnsbut_ = butgrp->addButton( "mnemonicsadd",
			tr("View/edit custom mnemonics"),
			mCB(this,uiWellMan,customMnsPush) );
    logedbut_ = butgrp->addButton( "edit", uiStrings::sEdit(),
			mCB(this,uiWellMan,editLogPush) );
    logsfld_->selectionChanged.notify( mCB(this,uiWellMan,logSel) );
    logsfld_->itemChosen.notify( mCB(this,uiWellMan,logSel) );
    butgrp->attach( rightOf, logsfld_->box() );
    logsgrp_->attach( rightOf, selgrp_ );

    welltrackbut_ = new uiToolButton( extrabutgrp_, "edwelltrack",
			uiStrings::phrEdit(mJoinUiStrs(sWell(),sTrack())),
			mCB(this,uiWellMan, edWellTrack) );

    if ( SI().zIsTime() )
    {
	csbut_ = new uiToolButton( extrabutgrp_, "checkshot",
			    uiStrings::phrEdit(tr("Checkshot Data")),
			    mCB(this,uiWellMan,edChckSh) );
	d2tbut_ = new uiToolButton( extrabutgrp_,
			    "z2t",uiStrings::phrEdit(tr("Depth/Time Model")),
			    mCB(this,uiWellMan, edD2T) );
    }

    markerbut_ = new uiToolButton( extrabutgrp_, "edmarkers",
			      uiStrings::phrEdit( uiStrings::sMarker(mPlural) ),
			      mCB(this,uiWellMan, edMarkers) );
    markerbut_->attach( rightOf, d2tbut_ ? d2tbut_ : welltrackbut_ );

    new uiToolButton( extrabutgrp_, "tools",
		      tr("Log tools"), mCB(this,uiWellMan,logTools) );
    extrabutgrp_->attach( ensureBelow, logsgrp_ );
    mTriggerInstanceCreatedNotifier();
}


uiWellMan::~uiWellMan()
{
    detachAllNotifiers();
}


void uiWellMan::ownSelChg()
{
    iswritable_ = curioobj_ && Well::Writer::isFunctional(*curioobj_);
    getCurrentWells();
    fillLogsFld();
    setWellToolButtonProperties();
    setLogToolButtonProperties();
}


void uiWellMan::getCurrentWells()
{
    curfnms_.erase();
    curwds_.erase();
    curmultiids_.erase();

    if ( !curioobj_ )
	return;

    const int nrsel = selGroup()->nrChosen();
    const Well::LoadReqs lreqs( Well::Inf );
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const IOObj* obj = IOM().get( selgrp_->chosenID(idx) );
	if ( !obj )
	    continue;

	const MultiID wid = obj->key();
	ConstRefMan<Well::Data> wd = Well::MGR().get( wid, lreqs );
	if ( !wd )
	    continue;

	curmultiids_ += wid;
	curfnms_.add( BufferString( obj->fullUserExpr(true) ) );
	curwds_.add( wd.getNonConstPtr() );
    }
}


void uiWellMan::copyPush( CallBacker* cb )
{
    uiCopyWellDlg dlg( this );
    if ( curioobj_ )
	dlg.setKey( curioobj_->key() );

    if ( dlg.go() == uiDialog::Accepted )
	updateCB( cb );
}


void uiWellMan::bulkD2TCB( CallBacker* )
{
    uiSetD2TFromOtherWell dlg( this );
    dlg.setSelected( getSelWells() );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    // update display?
}


#define mEnsureWellsSelected(act) \
    if ( curmultiids_.isEmpty() ) \
	act;

void uiWellMan::fillLogsFld()
{
    logsfld_->setEmpty();
    availablelognms_.setEmpty();
    defaultlognms_.setEmpty();
    mEnsureWellsSelected(return)

    const Well::LoadReqs lreqs( Well::LogInfos );
    const MultiID& key0 = curmultiids_.first();
    ConstRefMan<Well::Data> wd0 = Well::MGR().get( key0, lreqs );
    if ( wd0 )
    {
	wd0->logs().getNames( availablelognms_ );
	wd0->logs().getDefaultLogs( defaultlognms_ );
    }

    for ( int idx=1; idx<curmultiids_.size(); idx++ )
    {
	const MultiID& key = curmultiids_[idx];
	ConstRefMan<Well::Data> wd = Well::MGR().get( key, lreqs );
	if ( !wd )
	    continue;

	BufferStringSet lognms, deflognms;
	wd->logs().getNames( lognms );
	wd->logs().getDefaultLogs( deflognms );
	for ( int idy=0; idy<availablelognms_.size(); )
	{
	    if ( lognms.isPresent(availablelognms_.get(idy)) )
		idy++;
	    else
		availablelognms_.removeSingle( idy );
	}

	int index = 0;
	for ( auto* deflognm : defaultlognms_ )
	{
	    if ( deflognms.isPresent(*deflognm) )
		index++;
	    else
		defaultlognms_.removeSingle( index );
	}
    }

    logsfld_->addItems( availablelognms_ );
    logsfld_->chooseAll( false );
    addlogsbut_->setSensitive( iswritable_ && curmultiids_.size() == 1 );
    calclogsbut_->setSensitive( iswritable_ );
    setDefaultPixmaps();

    logSel( nullptr );
}


void uiWellMan::setDefaultPixmaps()
{
    for ( auto* deflognm : defaultlognms_ )
    {
	const int idx = logsfld_->indexOf( *deflognm );
	logsfld_->setMarked( idx, uiListBox::Pixmap );
    }
}


void uiWellMan::setButToolTip( uiButton* but, const uiString& oper,
			   const uiString& objtyp, const uiString& obj,
			   const uiString& end )
{
    if ( !but )
	return;

    uiString tt = toUiString("%1 %2").arg(oper)
				     .arg(objtyp);
    if ( but->sensitive() && !obj.isEmpty() )
	tt = tr("%1 for '%2'").arg(tt).arg(obj);

    if ( !end.isEmpty() )
	tt = toUiString("%1 %2").arg(tt).arg(end);

    but->setToolTip( tt );
}


void uiWellMan::updateLogsFld( CallBacker* )
{
    fillLogsFld();
}


void uiWellMan::calcClosedCB( CallBacker* )
{
    welllogcalcdlg_ = nullptr;
}

#define mSetWellButToolTip(but,objtyp) \
    setButToolTip( but, edvwstr, objtyp, curwellnm )


void uiWellMan::setWellToolButtonProperties()
{
    const uiString curwellnm = curioobj_ ? curioobj_->uiName() :
						      uiStrings::sEmptyString();
    const uiString edvwstr = iswritable_ ? uiStrings::sEdit() :
							     uiStrings::sView();

    mSetWellButToolTip( welltrackbut_, mJoinUiStrs(sWell(),sTrack()) );
    if ( d2tbut_ )
	mSetWellButToolTip( d2tbut_, tr("Depth/Time model") );

    if ( csbut_ )
	mSetWellButToolTip( csbut_, tr("Checkshot Data") );

    mSetWellButToolTip( markerbut_, uiStrings::sMarker(mPlural) );
}


#define mSetLogButToolTip(but,oper,end) \
    setButToolTip( but, oper, curlognm, curwellnm, end )


void uiWellMan::setLogToolButtonProperties()
{
    BufferStringSet wellnms, lognms;
    selGroup()->getChosen( wellnms );
    const int nrlogs = logsfld_->size();
    logsfld_->getChosen( lognms );

    const int nrchosenwells = curmultiids_.size();
    const int nrchosenlogs = lognms.size();
    const bool oneormorelog = nrchosenlogs > 0;

    logrenamebut_->setSensitive( iswritable_ && nrlogs > 0 );
    logrmbut_->setSensitive( iswritable_ && oneormorelog );
    logcopybut_->setSensitive( iswritable_ && nrlogs>0 );
    logexpbut_->setSensitive( oneormorelog );
    loguombut_->setSensitive( iswritable_ && nrlogs > 0 );
    logedbut_->setSensitive( iswritable_ && nrlogs > 0 );

    const uiString curwellnm = curioobj_ ? curioobj_->uiName() :
						      uiStrings::sEmptyString();
    const uiString curlognm = toUiString(logsfld_->getText());

    mSetLogButToolTip( logrenamebut_, uiStrings::sRename(),
						    uiStrings::sEmptyString() );
    mSetLogButToolTip( loguombut_, tr("View/edit units of measure for "),
						    uiStrings::sEmptyString() );
    mSetLogButToolTip( logedbut_, uiStrings::sEdit(),
						    uiStrings::sEmptyString() );

    setButToolTip(logrmbut_, uiStrings::sRemove(),
		  toUiString(lognms.getDispString(3)), curwellnm,
		  uiStrings::sEmptyString());
    setButToolTip(logcopybut_, uiStrings::sCopy(),
		  toUiString(lognms.getDispString(3)),
		  nrchosenwells==1 ? curwellnm : uiStrings::sEmptyString(),
		  uiStrings::sEmptyString() );
    setButToolTip(logexpbut_, uiStrings::sExport(),
		  toUiString(lognms.getDispString(3)),
		  nrchosenwells==1 ? curwellnm : uiStrings::sEmptyString(),
		  uiStrings::sEmptyString() );

    const int nrlogs2vw = nrchosenwells * nrchosenlogs ;
    const bool canview = nrlogs2vw >= 1;
    logvwbut_->setSensitive( canview );

    if ( canview )
    {
	const uiString tt = tr("View %1 for %2")
			      .arg( lognms.getDispString(2) )
			      .arg( wellnms.getDispString(2) );
	logvwbut_->setToolTip( tt );
    }
    else
	logvwbut_->setToolTip( mJoinUiStrs(sView(),sLog(mPlural).toLower()) );
}


void uiWellMan::logSel( CallBacker* )
{
    setLogToolButtonProperties();
}


#define mErrRet(msg) \
{ uiMSG().error(msg); return; }


void uiWellMan::edMarkers( CallBacker* )
{
    if ( !curioobj_ )
	return;

    const MultiID curmid( curioobj_->key() );
    const Well::LoadReqs lreqs( Well::Trck, Well::D2T, Well::Mrkrs );
    RefMan<Well::Data> wd = Well::MGR().get( curmid, lreqs );
    if ( !wd )
    {
	uiMSG().error( tr("Markers not present in %1")
						     .arg(curioobj_->name()) );
	return;
    }

    if ( !iswritable_ )
    {
	uiMarkerViewDlg dlg( this, *wd );
	dlg.go();
	return;
    }

    const Well::MarkerSet origmarkers = wd->markers();

    wd->track().setName( curioobj_->name() );
    uiMarkerDlg dlg( this, wd->track(), wd->d2TModel() );
    dlg.setMarkerSet( wd->markers() );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    dlg.getMarkerSet( wd->markers() );
    Well::Writer wtr( curmid, *wd );
    if ( !wtr.putMarkers() )
    {
	uiMSG().error( tr("Cannot write new markers to disk") );
	wd->markers() = origmarkers;
    }

    wd->markerschanged.trigger();
}


void uiWellMan::edWellTrack( CallBacker* )
{
    if ( !curioobj_ )
	return;

    const MultiID curmid( curioobj_->key() );
    const Well::LoadReqs lreqs( Well::Trck );
    RefMan<Well::Data> wd = Well::MGR().get( curmid, lreqs );
    if ( !wd )
    {
	uiMSG().error( tr("Track data not present in %1").arg(
							curioobj_->name()) );
	return;
    }

    const bool notfound = !wd->info().isLoaded() && wd->track().isEmpty();
    if ( notfound && !uiMSG().askGoOn(tr("No track found. Continue editing?")) )
	return;

    const Well::Track origtrck = wd->track();
    const Coord origpos = wd->info().surfacecoord_;
    const float origgl = wd->info().groundelev_;

    uiWellTrackDlg dlg( this, *wd );
    if ( !dlg.go() || !iswritable_ )
	return;

    Well::Writer wtr( curmid, *wd );
    if ( !wtr.putInfoAndTrack() )
    {
	uiMSG().error( tr("Cannot write new track to disk") );
	wd->track() = origtrck;
	wd->info().surfacecoord_ = origpos;
	wd->info().groundelev_ = origgl;
    }

    Well::putUWI( wd->multiID(), wd->info().uwid_ );
    wd->trackchanged.trigger();
    mkFileInfo();
}


void uiWellMan::edD2T( CallBacker* )
{
    defD2T( false );
}


void uiWellMan::edChckSh( CallBacker* )
{
    defD2T( true );
}


void uiWellMan::defD2T( bool chkshot )
{
    if ( !curioobj_ )
	return;

    const MultiID curmid = curioobj_->key();
    const Well::LoadReqs lreqs( chkshot ? Well::CSMdl : Well::D2T );
    RefMan<Well::Data> wd =  Well::MGR().get( curmid, lreqs );
    if ( !wd )
    {
	uiString errmsg;
	const OD::String& nm = curioobj_->name();
	if ( chkshot )
	    errmsg = tr("Checkshot data not present in %1").arg( nm );
	else
	    errmsg = tr("Time-Depth data not present in %1").arg( nm );

	uiMSG().error( errmsg );
	return;
    }

    if ( !chkshot && !wd->d2TModel() )
	wd->setD2TModel( new Well::D2TModel );
    if ( chkshot && !wd->checkShotModel() )
	wd->setCheckShotModel( new Well::D2TModel );

    const float oldreplvel = wd->info().replvel_;
    Well::D2TModel* inpmdl = chkshot ? wd->checkShotModel() : wd->d2TModel();
    PtrMan<Well::D2TModel> origd2t;
    if ( inpmdl )
	origd2t = new Well::D2TModel( *inpmdl );

    uiD2TModelDlg dlg( this, *wd, chkshot );
    if ( dlg.go() != uiDialog::Accepted || !iswritable_ )
	return;

    uiString errmsg;
    Well::Writer wtr( curmid, *wd );
    if ( (!chkshot && !wtr.putD2T()) || (chkshot && !wtr.putCSMdl()) )
    {
	errmsg = tr("Cannot write new model to disk");
	Well::D2TModel* toput = origd2t.release();
	if ( chkshot )
	    wd->setCheckShotModel( toput );
	else
	{
	    wd->setD2TModel( toput );
	    wd->info().replvel_ = oldreplvel;
	}
    }
    else if ( !mIsEqual(oldreplvel,wd->info().replvel_,1e-2f) &&
	      !wtr.putInfo() )
    {
	if ( !errmsg.isEmpty() )
	    errmsg.append( tr("Cannot write new %1 to disk")
			    .arg(Well::Info::sReplVel()),true );
	wd->info().replvel_ = oldreplvel;
    }

    if ( !errmsg.isEmpty() )
	uiMSG().error( errmsg );

    wd->d2tchanged.trigger();
    wd->trackchanged.trigger();
    mkFileInfo();
}


#define mEnsureLogSelected(msgtxt) \
    if ( logsfld_->isEmpty() ) \
	return; \
    const int nrsellogs = logsfld_->nrChosen(); \
    if ( nrsellogs < 1 ) \
	mErrRet(msgtxt)


void uiWellMan::logTools( CallBacker* )
{
    mEnsureWellsSelected(return)
    mEnsureLogSelected( uiStrings::sNoLogSel() )

    BufferStringSet wellnms, lognms;
    selGroup()->getChosen( wellnms );
    logsfld_->getChosen( lognms );

    //TODO: pass curmultiids_ directly to uiWellLogToolWinMgr
    uiWellLogToolWinMgr tooldlg( this, &wellnms, &lognms );
    tooldlg.go();
    fillLogsFld();
}


void uiWellMan::importLogs( CallBacker* )
{
    if ( !curioobj_ )
	return;

    uiImportLogsDlg dlg( this, curioobj_, true );
    if ( dlg.go() == uiDialog::Accepted )
	wellsChgd();
}


void uiWellMan::calcLogs( CallBacker* )
{
    mEnsureWellsSelected(return)
    if ( !welllogcalcdlg_ )
    {
	welllogcalcdlg_ = new  uiWellLogCalc( this, curmultiids_ );
	welllogcalcdlg_->setModal( false );
	welllogcalcdlg_->setDeleteOnClose( true );
	mAttachCB( welllogcalcdlg_->logschanged, uiWellMan::updateLogsFld );
	mAttachCB( welllogcalcdlg_->windowClosed, uiWellMan::calcClosedCB );
    }

    if ( !welllogcalcdlg_->updateWells(curmultiids_) )
    {
	mDetachCB( welllogcalcdlg_->logschanged, uiWellMan::updateLogsFld );
	welllogcalcdlg_->close();
	return;
    }

    welllogcalcdlg_->show();
}


void uiWellMan::logUOMPush( CallBacker* )
{
    mEnsureWellsSelected(return)
    mEnsureLogSelected( uiStrings::sNoLogSel() )
    BufferStringSet lognms;
    logsfld_->getChosen( lognms );

    ManagedObjectSet<BufferStringSet> editedlognmsset;
    for ( int widx=0; widx<curmultiids_.size(); widx++ )
	editedlognmsset.add( new BufferStringSet() );

    uiWellLogUOMDlg dlg( this, curmultiids_, lognms, editedlognmsset );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    selGroup()->chooseAll( false );
    selGroup()->setChosen( curmultiids_ );
    uiRetVal uirv;
    BufferStringSet alleditedlognms;
    for ( int widx=0; widx<curmultiids_.size(); widx++ )
    {
	const MultiID& wid = curmultiids_[widx];
	if ( !editedlognmsset.validIdx(widx) )
	    continue;

	const BufferStringSet& editedlognms = *editedlognmsset.get( widx );
	if ( editedlognms.isEmpty() )
	    continue;

	const uiRetVal saveret =
			Well::MGR().writeLogHeaders( wid, editedlognms );
	if ( saveret.isError() )
	    uirv.add( saveret );

	alleditedlognms.add( editedlognms, false );
    }

    if ( uirv.isError() )
	uiMSG().errorWithDetails( uirv, tr("Cannot write all log headers") );

    if ( !alleditedlognms.isEmpty() )
	wellLogsChgd( alleditedlognms );
}


void uiWellMan::logMnemPush( CallBacker* )
{
    mEnsureWellsSelected(return)
    mEnsureLogSelected( uiStrings::sNoLogSel() )
    BufferStringSet lognms;
    logsfld_->getChosen( lognms );

    ManagedObjectSet<BufferStringSet> editedlognmsset;
    for ( int widx=0; widx<curmultiids_.size(); widx++ )
	editedlognmsset.add( new BufferStringSet() );

    uiWellLogMnemDlg dlg( this, curmultiids_, lognms, editedlognmsset );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    selGroup()->chooseAll( false );
    selGroup()->setChosen( curmultiids_ );
    uiRetVal uirv;
    BufferStringSet alleditedlognms;
    for ( int widx=0; widx<curmultiids_.size(); widx++ )
    {
	const MultiID& wid = curmultiids_[widx];
	const BufferStringSet& editedlognms = *editedlognmsset.get( widx );
	if ( editedlognms.isEmpty() )
	    continue;

	const uiRetVal saveret =
			Well::MGR().writeLogHeaders( wid, editedlognms );
	if ( saveret.isError() )
	    uirv.add( saveret );

	alleditedlognms.add( editedlognms, false );
    }

    if ( uirv.isError() )
	uiMSG().errorWithDetails( uirv, tr("Cannot write all log headers") );

    if ( !alleditedlognms.isEmpty() )
	wellLogsChgd( alleditedlognms );
}


void uiWellMan::defMnemLogPush( CallBacker* )
{
    mEnsureWellsSelected(return)

    uiWellDefMnemLogDlg dlg( this, curmultiids_ );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    wellsChgd();
}


void uiWellMan::customMnsPush( CallBacker* )
{
    uiCustomMnemonicsSel dlg( this );
    dlg.go();
}


void uiWellMan::editLogPush( CallBacker* )
{
    mEnsureWellsSelected(return)
    mEnsureLogSelected( uiStrings::sNoLogSel() )

    const MultiID& wid = curmultiids_.first();
    const int selidx = logsfld_->firstChosen();
    const char* lognm = logsfld_->textOfItem( selidx );
    const BufferStringSet lognms( lognm );
    const Well::LoadReqs lreqs( lognms );
    ConstRefMan<Well::Data> wd = Well::MGR().get( wid, lreqs );
    if ( !wd )
    {
	uiMSG().error( Well::MGR().errMsg() );
	return;
    }

    const Well::Log* log = wd->logs().getLog( lognm );
    if ( !log )
    {
	uiMSG().error( tr("Log not available or no values present") );
	return;
    }

    PtrMan<Well::Log> logcopy = new Well::Log( *log );
    uiWellLogEditor dlg( this, *logcopy );
    if ( dlg.go() != uiDialog::Accepted || !dlg.isLogChanged() )
	return;

    const bool res = uiMSG().askSave(
			tr("One or more log values have been changed."
			   "\n\nDo you want to save your changes?"), false );
    if ( !res )
	return;

    logcopy->updateAfterValueChanges();
    if ( !Well::MGR().writeAndRegister(wid,logcopy) )
    {
	uiMSG().error( Well::MGR().errMsg() );
	return;
    }

    wellLogsChgd( lognms );
}


void uiWellMan::writeLogs()
{
    for ( int idwell=0; idwell<curwds_.size(); idwell++ )
    {
	Well::Writer wwr( curmultiids_[idwell], *curwds_[idwell] );
	if ( !wwr.putLogs() )
	    uiMSG().error( wwr.errMsg() );
    }

    wellsChgd();
}


void uiWellMan::writeLog( const MultiID& key,
			  Well::Data& wd, const Well::Log& log )
{
    Well::Writer wwr( key, wd );
    if ( !wwr.putLog(log) )
	uiMSG().error( wwr.errMsg() );
}


void uiWellMan::wellsChgd()
{
    fillLogsFld();
}


void uiWellMan::wellLogsChgd( const BufferStringSet& lognms )
{
    fillLogsFld();
}


void uiWellMan::viewLogPush( CallBacker* )
{
    mEnsureWellsSelected(return)
    mEnsureLogSelected( uiStrings::sNoLogSel() )

    const DBKeySet wellkeys( curmultiids_ );
    BufferStringSet lognms;
    logsfld_->getChosen( lognms );

    const BufferString logstr = lognms.cat( "," );
    lognms.setEmpty();
    lognms.add( logstr );  //TODO No a valid way to forward a set of log names
    GetWellDisplayServer().createMultiWellDisplay( this, wellkeys, lognms );
}


void uiWellMan::renameLogPush( CallBacker* )
{
    mEnsureWellsSelected(return)
    mEnsureLogSelected( uiStrings::sNoLogSel() );

    const BufferString lognm = logsfld_->getText();
    const uiString titl = uiStrings::phrRename(toUiString("'%1'").arg(lognm));
    uiGenInputDlg dlg( this, titl, mJoinUiStrs(sNew(),sName().toLower()),
		       new StringInpSpec(lognm) );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    const BufferString newnm = dlg.text();
    if ( logsfld_->isPresent(newnm) )
	mErrRet( tr("Name already in use") )

    if ( !Well::MGR().renameLog(curmultiids_,lognm,newnm) )
	uiMSG().error( Well::MGR().errMsg() );

    fillLogsFld(); //If some have been renamed successfully
}


void uiWellMan::removeLogPush( CallBacker* )
{
    mEnsureWellsSelected(return)
    mEnsureLogSelected( uiStrings::sNoLogSel() );
    BufferStringSet lognms;
    logsfld_->getChosen( lognms );

    const uiString msg = tr("Selected logs will be permanently deleted."
			    "\nDo you wish to continue?");
    uiStringSet details;
    lognms.fill( details );
    const int res = uiMSG().askDeleteWithDetails( msg, details );
    if ( res == 0 )
	return;

    uiRetVal uirv;
    for ( const auto& wid : curmultiids_ )
    {
	if ( !Well::MGR().deleteLogs(wid,lognms) )
	    uirv.add( Well::MGR().errMsg() );
    }

    if ( uirv.isError() )
	uiMSG().errorWithDetails( uirv, tr("Cannot remove all requested logs"));

    fillLogsFld(); //If some have been removed successfully
}


void uiWellMan::copyLogPush( CallBacker* )
{
    mEnsureWellsSelected(return)
    mEnsureLogSelected( uiStrings::sNoLogSel() );
    BufferStringSet lognms;
    logsfld_->getChosen( lognms );

    const Well::LoadReqs lreqs( lognms );
    uiRetVal uirv;
    for ( const auto& wid : curmultiids_ )
    {
	ConstRefMan<Well::Data> wd = Well::MGR().get( wid, lreqs );
	if ( !wd )
	{
	    uirv.add( Well::MGR().errMsg() );
	    continue;
	}

	const Well::LogSet& logs = wd->logs();
	ManagedObjectSet<Well::Log> copiedlogs;
	BufferStringSet addedlognms;
	for ( const auto* logname : lognms )
	{
	    BufferString baselognm( "Copy of ", logname->buf() );
	    BufferString copylognm( baselognm );
	    int count = 0;
	    while ( logs.isPresent(copylognm) &&
		    addedlognms.isPresent(copylognm) )
	    {
		count++;
		copylognm = baselognm;
		copylognm.add( "(" ).add( count ).add( ")" );
	    }

	    const Well::Log* log = logs.getLog( logname->buf() );
	    if ( !log )
		continue; //checked above, should not happen

	    auto* copylog = new Well::Log( *log );
	    copylog->setName( copylognm );
	    copiedlogs.add( copylog );
	    addedlognms.add( copylognm );
	}

	if ( !Well::MGR().writeAndRegister(wid,copiedlogs) )
	    uirv.add( Well::MGR().errMsg() );
    }

    fillLogsFld();
}


void uiWellMan::exportLogs( CallBacker* )
{
    mEnsureWellsSelected(return)
    mEnsureLogSelected( uiStrings::sNoLogSel() );
    BufferStringSet lognms;
    logsfld_->getChosen( lognms );

    uiExportLogs dlg( this, curmultiids_, lognms );
    dlg.go();
}


#define mAddWellInfo(key,str) \
    if ( !str.isEmpty() ) \
    { txt.add( key.getFullString() ).add( colonstr ).add( str ).addNewLine(); }


void uiWellMan::mkFileInfo()
{
    BufferString txt( "<No specific info available>\n" );
    if ( !curioobj_ )
    {
	setInfo( txt );
	return;
    }

    const Well::LoadReqs lreqs( Well::Inf, Well::Trck );
    ConstRefMan<Well::Data> wd = Well::MGR().get( curioobj_->key(), lreqs );
    if ( !wd )
    {
	setInfo( txt );
	return;
    }

    txt.setEmpty();
    const Well::Info& info = wd->info();
    const Well::Track& track = wd->track();

    StringView colonstr( ": " );
    const BufferString posstr(
	    info.surfacecoord_.toPrettyString(SI().nrXYDecimals()), " - ",
	    SI().transform(info.surfacecoord_).toString() );
    mAddWellInfo(Well::Info::sCoord(),posstr)

    if ( !track.isEmpty() )
    {
	const float rdelev = track.getKbElev();
	const UnitOfMeasure* zun = UnitOfMeasure::surveyDefDepthUnit();
	if ( !mIsZero(rdelev,1e-4) && !mIsUdf(rdelev) )
	{
	    txt.add( Well::Info::sKeyKBElev() ).add( colonstr );
	    txt.add( toString(zun ? zun->userValue(rdelev)
				  : rdelev,0,'f',2) );
	    if ( zun )
		txt.add( zun->symbol() );
	    txt.addNewLine();
	}

	const float td = track.dahRange().stop_;
	if ( !mIsZero(td,1e-3f) && !mIsUdf(td) )
	{
	    txt.add(Well::Info::sKeyTD()).add( colonstr );
	    txt.add( toString(zun ? zun->userValue(td) : td,0,'f',2) );
	    if ( zun )
		txt.add( zun->symbol() );
	    txt.addNewLine();
	}

	const double srd = SI().seismicReferenceDatum();
	if ( !mIsZero(srd,1e-4) )
	{
	    txt.add( SurveyInfo::sKeySeismicRefDatum() ).add( colonstr );
	    txt.add( toString(zun ? zun->userValue(srd) : srd,0,'f',2) );
	    if ( zun )
		txt.add( zun->symbol() );
	    txt.addNewLine();
	}

	const float replvel = info.replvel_;
	if ( !mIsUdf(replvel) )
	{
	     txt.add( Well::Info::sKeyReplVel() ).add( colonstr );
	     txt.add( zun ? zun->userValue(replvel) : replvel );
	     txt.add( UnitOfMeasure::surveyDefVelUnitAnnot(true,false)
		      .getFullString() );
	     txt.addNewLine();
	}

	const float groundelev = info.groundelev_;
	if ( !mIsUdf(groundelev) )
	{
	    txt.add( Well::Info::sKeyGroundElev() ).add( colonstr );
	    txt.add( toString(zun ? zun->userValue(groundelev)
				  : groundelev,0,'f',2) );
	    if ( zun )
		txt.add( zun->symbol() );
	    txt.addNewLine();
	}
    }

    mAddWellInfo(Well::Info::sUwid(),info.uwid_)
    mAddWellInfo(Well::Info::sOper(),info.oper_)
    mAddWellInfo(Well::Info::sCounty(),info.county_)
    mAddWellInfo(Well::Info::sState(),info.state_)
    mAddWellInfo(Well::Info::sCountry(),info.country_)

    txt.add( getFileInfo() );
    setInfo( txt );
}


const BufferStringSet& uiWellMan::getAvailableLogs() const
{
    return availablelognms_;
}
