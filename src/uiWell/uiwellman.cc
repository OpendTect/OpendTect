/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/

#include "uiwellman.h"

#include "bufstringset.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "file.h"
#include "ptrman.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "welltrack.h"
#include "wellinfo.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "wellmanager.h"
#include "welltransl.h"
#include "wellwriter.h" // for Well::Writer::isFunctional

#include "uitoolbutton.h"
#include "uigeninputdlg.h"
#include "uigroup.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitextedit.h"
#include "uitoolbar.h"
#include "uiwelldlgs.h"
#include "uiwelllogimpexp.h"
#include "uiwelllogcalc.h"
#include "uiwelllogtools.h"
#include "uiwellmarkerdlg.h"
#include "uiwelllogdisplay.h"
#include "od_helpids.h"

mDefineInstanceCreatedNotifierAccess(uiWellMan)


uiWellMan::uiWellMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup(
		uiStrings::phrManage( uiStrings::sWell(mPlural)),mNoDlgTitle,
				      mODHelpKey(mWellManHelpID))
				     .nrstatusflds(1).modal(false),
				    WellTranslatorGroup::ioContext(),
				    WellTranslatorGroup::sGroupName())
    , d2tbut_(0)
    , csbut_(0)
    , curiswritable_(true)
{
    createDefaultUI( false, true, true );
    setPrefWidth( 50 );

    if ( SI().zIsTime() )
    {
	uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
	manipgrp->addButton( "z2t",
			tr("Set Depth to Time Model from other Well"),
			mCB(this,uiWellMan,bulkD2TCB) );
    }

    logsgrp_ = new uiGroup( listgrp_, "Logs group" );
    uiListBox::Setup su( OD::ChooseAtLeastOne, uiStrings::sLogs(),
			 uiListBox::AboveMid );
    logsfld_ = new uiListBox( logsgrp_, su );
    logsfld_->setHSzPol( uiObject::Wide );

    uiButtonGroup* logsbgrp = new uiButtonGroup( listgrp_, "Logs buttons",
						 OD::Horizontal );
    addlogsbut_ = uiButton::getStd( logsbgrp, OD::Import,
				    mCB(this,uiWellMan,importLogs), false );
    calclogsbut_ = uiButton::getStd( logsbgrp, OD::Create,
				     mCB(this,uiWellMan,calcLogs), false );
    logsbgrp->attach( centeredBelow, logsgrp_ );

    uiManipButGrp* butgrp = new uiManipButGrp( logsfld_ );
    logvwbut_ = butgrp->addButton( "view_log", tr("View selected log"),
					      mCB(this,uiWellMan,viewLogPush) );
    logrenamebut_ = butgrp->addButton( uiManipButGrp::Rename,
		      uiStrings::phrRename(uiStrings::sSelectedLog().toLower()),
		      mCB(this,uiWellMan,renameLogPush) );
    logrmbut_ = butgrp->addButton( uiManipButGrp::Remove,
		uiStrings::phrRemove(uiStrings::sSelectedLog(mPlural).toLower())
		, mCB(this,uiWellMan,removeLogPush) );
    logexpbut_ = butgrp->addButton( "export",
			uiStrings::phrExport( uiStrings::sWellLog(mPlural) ),
			mCB(this,uiWellMan,exportLogs) );
    loguombut_ = butgrp->addButton( "unitsofmeasure",
					tr("View/edit unit of measure"),
					mCB(this,uiWellMan,logUOMPush) );
    logedbut_ = butgrp->addButton( "edit", uiStrings::sEdit(),
			mCB(this,uiWellMan,editLogPush) );
    logupbut_ = butgrp->addButton( "uparrow", uiStrings::sMoveUp(),
			mCB(this,uiWellMan,moveLogPush) );
    logdownbut_ = butgrp->addButton( "downarrow", uiStrings::sMoveDown(),
			mCB(this,uiWellMan,moveLogPush) );
    logsfld_->selectionChanged.notify( mCB(this,uiWellMan,logSel) );
    logsfld_->itemChosen.notify( mCB(this,uiWellMan,logSel) );
    butgrp->attach( rightOf, logsfld_->box() );
    logsgrp_->attach( rightOf, selgrp_ );

    welltrackbut_ = new uiToolButton( extrabutgrp_, "edwelltrack",
			uiStrings::phrEdit(uiStrings::sWellTrack()),
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

    mAttachCB( selgrp_->itemChosen, uiWellMan::itmChosenCB );
    selChg( this );
    mTriggerInstanceCreatedNotifier();
}


uiWellMan::~uiWellMan()
{
}


void uiWellMan::ownSelChg()
{
    curiswritable_ = curioobj_ && Well::Writer::isFunctional(*curioobj_);

    selwellids_.setEmpty();
    const int nrsel = selGroup()->nrChosen();
    for ( int idx=0; idx<nrsel; idx++ )
	selwellids_ += selgrp_->chosenID( idx );
    setWellToolButtonProperties();

    fillLogsFld();
    setLogToolButtonProperties();
}


void uiWellMan::bulkD2TCB( CallBacker* )
{
    uiSetD2TFromOtherWell dlg( this );
    dlg.setSelected( getSelWells() );
    if ( !dlg.go() )
	return;

    // update display?
}


void uiWellMan::fillLogsFld()
{
    logsfld_->setEmpty();
    const int nrselwells = selwellids_.size();
    if ( nrselwells < 1 )
	return;

    avlognms_.erase();
    Well::MGR().getLogNames( selwellids_[0], avlognms_ );

    for ( int iwell=1; iwell<nrselwells; iwell++ )
    {
	BufferStringSet lognms;
	Well::MGR().getLogNames( selwellids_[iwell], lognms );
	for ( int ilog=0; ilog<avlognms_.size(); ilog++ )
	{
	    if ( !lognms.isPresent(avlognms_.get(ilog)) )
	    {
		avlognms_.removeSingle( ilog );
		ilog--;
	    }
	}
    }

    for ( int ilog=0; ilog<avlognms_.size(); ilog++ )
	logsfld_->addItem( toUiString(avlognms_.get(ilog)) );

    logsfld_->chooseAll( false );
    addlogsbut_->setSensitive( curiswritable_ && nrselwells == 1 );
    calclogsbut_->setSensitive( curiswritable_ );

    setLogToolButtonProperties();
}



void uiWellMan::setButToolTip( uiButton* but, const uiString& oper,
			   const uiString& objtyp, const uiString& obj,
			   const uiString& end )
{
    if ( !but )
	return;

    uiString tt = toUiString("%1 %2").arg(oper)
				     .arg(objtyp);
    if ( but->isSensitive() && !obj.isEmpty() )
	tt = tr("%1 for '%2'").arg(tt).arg(obj);

    if ( !end.isEmpty() )
	tt = toUiString("%1 %2").arg(tt).arg(end);

    but->setToolTip( tt );
}


#define mSetWellButToolTip(but,objtyp) \
    setButToolTip( but, edvwstr, objtyp, curwellnm )
#define mWellNmUiStr(ioobj) ioobj ? toUiString(ioobj->name()) \
				  : uiString::empty()


void uiWellMan::setWellToolButtonProperties()
{
    const uiString curwellnm = mWellNmUiStr( curioobj_ );
    const uiString edvwstr = curiswritable_ ? uiStrings::sEdit() :
							     uiStrings::sView();

    mSetWellButToolTip( welltrackbut_, uiStrings::sWellTrack() );
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
    const int nrlogs = avlognms_.size();
    const int curidx = logsfld_->currentItem();

    BufferStringSet lognms; logsfld_->getChosen( lognms );
    const int nrchosenlogs = lognms.size();
    const int nrchosenwells = selwellids_.size();
    const bool oneormorelog = nrchosenlogs > 0;
    const bool singlewellsel = nrchosenwells == 1;
    const bool singlewritable = singlewellsel && curiswritable_;

    logdownbut_->setSensitive( singlewritable && curidx>=0 && curidx<nrlogs-1 );
    logupbut_->setSensitive( singlewritable && curidx>0 );
    logedbut_->setSensitive( singlewritable && nrlogs > 0 );
    logrenamebut_->setSensitive( curiswritable_ && nrlogs > 0 );
    logrmbut_->setSensitive( curiswritable_ && oneormorelog );
    logexpbut_->setSensitive( oneormorelog );
    loguombut_->setSensitive( curiswritable_ && nrlogs > 0 );

    const uiString curwellnm = mWellNmUiStr( curioobj_ );
    const uiString curlognm = toUiString( logsfld_->getText() );

    mSetLogButToolTip( logupbut_, uiStrings::sMove(),
						   uiStrings::sUp().toLower() );
    mSetLogButToolTip( logdownbut_, uiStrings::sMove(),
						 uiStrings::sDown().toLower() );
    mSetLogButToolTip( logrenamebut_, uiStrings::sRename(), uiString::empty() );
    mSetLogButToolTip( loguombut_, tr("View/edit units of measure for "),
						    uiString::empty() );
    mSetLogButToolTip( logedbut_, uiStrings::sEdit(), uiString::empty() );

    setButToolTip(logrmbut_, uiStrings::sRemove(),
		  toUiString(lognms.getDispString(3)), curwellnm,
		  uiString::empty());
    setButToolTip(logexpbut_, uiStrings::sExport(),
		  toUiString(lognms.getDispString(3)),
		  nrchosenwells==1 ? curwellnm : uiString::empty(),
		  uiString::empty() );

    const int nrlogs2vw = nrchosenwells * nrchosenlogs ;
    const bool canview = nrlogs2vw == 1 || nrlogs2vw == 2;
    logvwbut_->setSensitive( canview );

    if ( !canview )
	logvwbut_->setToolTip( tr("View logs") );
    else
    {
	BufferStringSet wellnms;
	for ( int iwell=0; iwell<selwellids_.size(); iwell++ )
	{
	    IOObj* ioobj = selwellids_[iwell].getIOObj();
	    if ( ioobj )
		wellnms.add( ioobj->name() );
	    delete ioobj;
	}

	uiString tt = tr("View %1 for %2")
		      .arg(toUiString(lognms.getDispString(2)))
		      .arg(toUiString(wellnms.getDispString(2)));
	logvwbut_->setToolTip( tt );
    }
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
    RefMan<Well::Data> wd = getWellData( curioobj_->key(), true, Well::Trck,
					 Well::Mrkrs );
    if ( !wd )
	return;

    if ( !curiswritable_ )
    {
	uiMarkerViewDlg dlg( this, *wd );
	dlg.go(); return;
    }

    uiMarkerDlg dlg( this, wd->track() );
    dlg.setMarkerSet( wd->markers() );
    if ( !dlg.go() || !curiswritable_ )
	return;

    saveWell( *wd );
}


void uiWellMan::edWellTrack( CallBacker* )
{
    if ( !curioobj_ )
	return;
    RefMan<Well::Data> wd = getWellData( curioobj_->key(), true, Well::Trck );
    if ( !wd )
	return;

    uiWellTrackDlg dlg( this, *wd );
    if ( !dlg.go() || !curiswritable_ )
	return;

    saveWell( *wd );
    updateFromSelected();
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
    RefMan<Well::Data> wd = getWellData( curioobj_->key(), true, Well::Trck,
					 chkshot ? Well::CSMdl : Well::D2T );
    if ( !wd )
	return;

    uiD2TModelDlg dlg( this, *wd, chkshot );
    if ( !dlg.go() || !curiswritable_ )
	return;

    saveWell( *wd );
    updateFromSelected();
}


void uiWellMan::logTools( CallBacker* )
{
    BufferStringSet wellnms, lognms;
    logsfld_->getChosen( lognms );
    for ( int iobj=0; iobj<selwellids_.size(); iobj++ )
	wellnms.add( selwellids_[iobj].name() );

    uiUserShowWait usw( this, uiStrings::sCollectingData() );
    uiWellLogToolWinMgr tooldlg( this, &wellnms, &lognms );
    usw.readyNow();
    tooldlg.go();
}


#define mEmptyLogs(idx) \
    if ( curwds_.validIdx(idx) ) \
	curwds_[idx]->logs().setEmpty()

void uiWellMan::importLogs( CallBacker* )
{
    uiImportLogsDlg dlg( this, curioobj_ );
    dlg.go();
    if ( dlg.go() )
	fillLogsFld();
}


void uiWellMan::calcLogs( CallBacker* )
{
    if ( avlognms_.isEmpty() )
	return;

    uiWellLogCalc dlg( this, selwellids_ );
    dlg.go();
    if ( dlg.haveNewLogs() )
	fillLogsFld();
}


void uiWellMan::logUOMPush( CallBacker* )
{
    if ( selwellids_.isEmpty() )
	return;

    BufferStringSet lognms;
    logsfld_->getChosen( lognms );
    if ( lognms.isEmpty() )
	mErrRet( uiStrings::sNoLogSel() )

    BufferStringSet wellnms;
    ObjectSet<Well::LogSet> wls;
    TypeSet< RefMan<Well::Data> > wds;
    for ( int iwell=0; iwell<selwellids_.size(); iwell++ )
    {
	RefMan<Well::Data> wd = getWellData( selwellids_[iwell], false,
					     Well::Logs );
	if ( wd )
	{
	    wds += wd;
	    wls += &wd->logs();
	    wellnms.add( wd->name() );
	}
    }

    uiWellLogUOMDlg dlg( this, wls, wellnms, lognms );
    if ( !dlg.go() )
	return;

    uiUserShowWait usw( this, uiStrings::sSavingData() );
    for ( int iwell=0; iwell<wds.size(); iwell++ )
	saveWell( *wds[iwell], false );
}


void uiWellMan::editLogPush( CallBacker* )
{
    if ( selwellids_.isEmpty() )
	return;
    const int selidx = logsfld_->firstChosen();
    if ( selidx < 0 )
	mErrRet( uiStrings::sNoLogSel() )
    RefMan<Well::Data> wd = getWellData( selwellids_[0], true,
					 Well::Trck, Well::Logs );
    if ( !wd )
	return;

    const char* lognm = logsfld_->itemText( selidx );
    Well::LogSet& wls = wd->logs();
    Well::Log* wl = wls.getLogByName( lognm );
    if ( !wl )
	mErrRet( uiStrings::phrCannotRead(uiStrings::sWellLog()) )

    Well::Log* log4edit = wl->clone();

    uiWellLogEditor dlg( this, *log4edit );
    if ( !dlg.go() || !dlg.isLogChanged() )
	return;

    *wl = *log4edit;

    if ( uiMSG().askSave(tr("Save the changes to disk now?"),false) )
	saveWell( *wd );
}


RefMan<Well::Data> uiWellMan::getWellData( DBKey dbky, bool emiterr,
			Well::SubObjType typ1, Well::SubObjType typ2 )
{
    RefMan<Well::Data> ret;
    if ( !dbky.isValid() )
	return ret;

    uiRetVal uirv;
    Well::LoadReqs reqs( Well::Inf );
    if ( (typ1 == Well::Trck || typ2 == Well::Trck) && SI().zIsTime() )
	reqs.add( Well::D2T );
    if ( typ1 != Well::Inf )
	reqs.add( typ1 );
    if ( typ2 != Well::Inf )
	reqs.add( typ2 );

    uiUserShowWait usw( this, uiStrings::sSavingData() );
    ret = Well::MGR().fetchForEdit( dbky, reqs, uirv );
    if ( emiterr && !ret )
    {
	usw.readyNow();
	uiMSG().error( uirv );
    }

    return ret;
}


void uiWellMan::saveWell( const Well::Data& wd, bool showwait )
{
    PtrMan<uiUserShowWait> usw = showwait
	? new uiUserShowWait(this,uiStrings::sSavingData()) : 0;

    SilentTaskRunnerProvider trprov;
    uiRetVal uirv = Well::MGR().store( wd, trprov );
    if ( usw )
	usw->readyNow();

    if ( !uirv.isOK() )
	uiMSG().error( uirv );
}


void uiWellMan::moveLogPush( CallBacker* cb )
{
    if ( selwellids_.isEmpty() )
	return;
    mDynamicCastGet(uiToolButton*,toolbut,cb);
    if ( toolbut != logupbut_ && toolbut != logdownbut_ )
	return;

    const bool isup = toolbut == logupbut_;
    const int curlogidx = logsfld_->currentItem();
    const int newlogidx = curlogidx + ( isup ? -1 : 1 );
    RefMan<Well::Data> wd = getWellData( selwellids_[0], true, Well::Logs );
    if ( !wd )
	return;

    if ( wd->logs().swap(curlogidx,newlogidx) )
    {
	saveWell( *wd );
	logsfld_->setCurrentItem( newlogidx );
    }

    fillLogsFld();
}


#define mEnsureLogSelected(msgtxt) \
    if ( avlognms_.isEmpty() ) return; \
    const int nrsel = logsfld_->nrChosen(); \
    if ( nrsel < 1 ) \
	mErrRet(msgtxt)


void uiWellMan::viewLogPush( CallBacker* )
{
    mEnsureLogSelected( uiStrings::sNoLogSel() )
    RefMan<Well::Data> wd1 = getWellData( selwellids_[0], true, Well::Logs );
    if ( !wd1 )
	return;

    const Well::LogSet& wls1 = wd1->logs();
    const char* lognm = logsfld_->itemText( logsfld_->firstChosen() );
    const Well::Log* wl1 = wls1.getLogByName( lognm );
    if ( !wl1 )
	mErrRet( uiStrings::phrCannotRead( uiStrings::sWellLog() ) )

    BufferStringSet lognms;
    logsfld_->getChosen( lognms );
    const int maxnrchosen = selwellids_.size() * lognms.size();
    if ( maxnrchosen < 1 || maxnrchosen > 2 )
	mErrRet( tr("Please select a maximum of 2 logs") );

    BufferStringSet wnms;
    wnms.add( wd1->name() );
    const Well::Log* wl2 = 0;
    if ( lognms.size() > 1 )
	wl2 = wls1.getLogByName( lognms.get(1) );

    RefMan<Well::Data> wd2;
    if ( !wl2 && selwellids_.size() > 1 )
    {
	wd2 = getWellData( selwellids_[1], true, Well::Logs );
	if ( wd2 )
	{
	    wl2 = wd2->logs().getLogByName( lognms.get(0) );
	    if ( wl2 )
		wnms.add( wd2->name() );
	}
    }

    if ( wnms.size() == 1 )
	(void)uiWellLogDispDlg::popupNonModal( this, wl1, wl2,
					       wnms.get(0), 0 );
    else if ( wnms.size() == 2 )
	(void)uiWellLogDispDlg::popupNonModal( this, wl1, wl2,
					       wnms.get(0), wnms.get(1) );
}


void uiWellMan::renameLogPush( CallBacker* )
{
    mEnsureLogSelected(uiStrings::sNoLogSel());
    BufferString lognm = logsfld_->getText();
    const uiString titl = uiStrings::phrRename(toUiString("'%1'").arg(lognm));
    uiGenInputDlg dlg( this, titl, uiStrings::sNewName().toLower(),
				new StringInpSpec(lognm));
    if ( !dlg.go() )
	return;

    BufferString newnm = dlg.text();
    if ( logsfld_->isPresent(newnm) )
	mErrRet(tr("Name already in use"))

    uiUserShowWait usw( this, uiStrings::sSavingData() );
    for ( int iwell=0; iwell<selwellids_.size(); iwell++ )
    {
	RefMan<Well::Data> wd = getWellData( selwellids_[iwell], false,
					     Well::Logs );
	if ( wd )
	{
	    Well::Log* log = wd->logs().getLogByName( lognm );
	    if ( log )
		log->setName( newnm );
	    saveWell( *wd, false );
	}
    }

    fillLogsFld();
}


void uiWellMan::removeLogPush( CallBacker* )
{
    mEnsureLogSelected(uiStrings::sNoLogSel());

    uiString msg;
    msg = tr("%1will be removed from disk.\nDo you wish to continue?")
	.arg(nrsel == 1 ? tr("This log ") : tr("These logs "));
    if ( !uiMSG().askRemove(msg) )
	return;

    BufferStringSet logs2rem; logsfld_->getChosen( logs2rem );

    uiUserShowWait usw( this, uiStrings::sSavingData() );
    for ( int iwell=0; iwell<selwellids_.size(); iwell++ )
    {
	RefMan<Well::Data> wd = getWellData( selwellids_[iwell], false,
					     Well::Logs );
	if ( wd )
	{
	    for ( int ilog=0; ilog<logs2rem.size(); ilog++ )
		wd->logs().removeByName( logs2rem.get(ilog) );
	    saveWell( *wd, false );
	}
    }

    fillLogsFld();
}


void uiWellMan::exportLogs( CallBacker* )
{
    mEnsureLogSelected( uiStrings::sNoLogSel() );

    TypeSet< RefMan<Well::Data> > wds; // to keep refs alive
    ObjectSet<Well::Data> expwds;
    for ( int iwell=0; iwell<selwellids_.size(); iwell++ )
    {
	RefMan<Well::Data> wd = getWellData( selwellids_[iwell], false,
					     Well::Trck, Well::Logs );
	if ( wd )
	{
	    wds += wd;
	    expwds += wd.ptr();
	}
    }
    BufferStringSet sellogs; logsfld_->getChosen( sellogs );

    uiExportLogs dlg( this, expwds, sellogs );
    dlg.go();
}


bool uiWellMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    uiRetVal readresult;
    const bool survintime = SI().zIsTime();
    Well::LoadReqs reqs( Well::Trck, Well::Inf );
    if ( survintime )
	reqs.add( Well::D2T );
    ConstRefMan<Well::Data> curwd = Well::MGR().fetch( ioobj.key(), reqs,
							readresult );

    if ( !curwd )
	{ inf = readresult; return false; }

    const Well::Info& info = curwd->info();
    const Well::Track& track = curwd->track();

    const BinID surfbid = SI().transform( info.surfaceCoord() );
    addObjInfo( inf, Well::Info::sCoord(),
		     toUiString(info.surfaceCoord().toString()) )
	     .postFixWord( toUiString(surfbid.toString()) );

#   define mAddZBasedInfo( key, val ) \
	addObjInfo( inf, key, toUiString(val) ).withUnit( zunstr )

    if ( !track.isEmpty() )
    {
	const uiString zunstr = UnitOfMeasure::surveyDefDepthUnitAnnot( true );

	const float rdelev = track.getKbElev();
	if ( !mIsZero(rdelev,1e-4) && !mIsUdf(rdelev) )
	    mAddZBasedInfo( Well::Info::sKBElev(), rdelev );

	const float td = track.dahRange().stop;
	if ( !mIsZero(td,1e-3f) && !mIsUdf(td) )
	    mAddZBasedInfo( Well::Info::sTD(), td );

	const double srd = SI().seismicReferenceDatum();
	if ( !mIsZero(srd,1e-4) )
	    mAddZBasedInfo( SurveyInfo::sSeismicRefDatum(), srd );

	const float replvel = info.replacementVelocity();
	if ( !mIsUdf(replvel) )
	    addObjInfo( inf, Well::Info::sReplVel(), replvel )
			.withUnit( toUiString("m/s") );

	const float groundelev = info.groundElevation();
	if ( !mIsUdf(groundelev) )
	    mAddZBasedInfo( Well::Info::sGroundElev(), groundelev );

	if ( survintime && !curwd->haveD2TModel() )
	    inf.add( toUiString("** %1\n\t%2")
		    .arg( tr("No valid Depth vs Time relation") )
		    .arg( tr("Use 'Tie Well To Seismics' to add one") ) );
    }

#define mAddIfNotEmpty(wikey,fn) \
    if ( !info.fn().isEmpty() ) \
	addObjInfo( inf, Well::Info::wikey(), info.fn() )

    mAddIfNotEmpty( sUwid, UWI );
    mAddIfNotEmpty( sOper, wellOperator );
    mAddIfNotEmpty( sState, getState );
    mAddIfNotEmpty( sCounty, getCounty );

    return true;
}
