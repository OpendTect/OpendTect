/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwellman.h"

#include "bufstringset.h"
#include "ioobj.h"
#include "ioman.h"
#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "wellman.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "welltransl.h"
#include "wellwriter.h"

#include "uitoolbutton.h"
#include "uigeninputdlg.h"
#include "uigroup.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
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
    : uiObjFileMan(p,uiDialog::Setup("Manage Wells",mNoDlgTitle,
				    mODHelpKey(mWellManHelpID)).nrstatusflds(1),
	           WellTranslatorGroup::ioContext() )
    , d2tbut_(0)
    , csbut_(0)
{
    createDefaultUI( false, true, true );
    setPrefWidth( 50 );

    logsgrp_ = new uiGroup( listgrp_, "Logs group" );
    uiLabel* lbl = new uiLabel( logsgrp_, "Logs" );
    logsfld_ = new uiListBox( logsgrp_, "Available logs",
				OD::ChooseAtLeastOne );
    logsfld_->attach( alignedBelow, lbl );

    uiButtonGroup* logsbgrp = new uiButtonGroup( listgrp_, "Logs buttons",
						 OD::Horizontal );
    addlogsbut_ = new uiPushButton( logsbgrp, "&Import", false );
    addlogsbut_->activated.notify( mCB(this,uiWellMan,importLogs) );
    calclogsbut_ = new uiPushButton( logsbgrp, "&Create", false );
    calclogsbut_->activated.notify( mCB(this,uiWellMan,calcLogs) );
    calclogsbut_->attach( rightOf, addlogsbut_ );
    logsbgrp->attach( centeredBelow, logsgrp_ );

    uiManipButGrp* butgrp = new uiManipButGrp( logsgrp_ );
    logvwbut_ = butgrp->addButton( "view_log", "View selected log",
			mCB(this,uiWellMan,viewLogPush) );
    logrenamebut_ = butgrp->addButton( uiManipButGrp::Rename,
		"Rename selected log", mCB(this,uiWellMan,renameLogPush) );
    logrmbut_ = butgrp->addButton( uiManipButGrp::Remove,
		"Remove selected log(s)", mCB(this,uiWellMan,removeLogPush) );
    logexpbut_ = butgrp->addButton( "export", "Export log(s)",
			mCB(this,uiWellMan,exportLogs) );
    loguombut_ = butgrp->addButton( "unitsofmeasure",
		"View/edit unit of measure", mCB(this,uiWellMan,logUOMPush) );
    logupbut_ = butgrp->addButton( "uparrow", "Move up",
			mCB(this,uiWellMan,moveLogsPush) );
    logdownbut_ = butgrp->addButton( "downarrow", "Move down",
			mCB(this,uiWellMan,moveLogsPush) );
    logsfld_->selectionChanged.notify( mCB(this,uiWellMan,logSel) );
    logsfld_->itemChosen.notify( mCB(this,uiWellMan,logSel) );
    butgrp->attach( rightOf, logsfld_ );
    logsgrp_->attach( rightOf, selgrp_ );

    welltrackbut_ = new uiToolButton( listgrp_, "edwelltrack",
		"Edit Well Track", mCB(this,uiWellMan, edWellTrack) );
    welltrackbut_->attach( alignedBelow, selgrp_ );
    welltrackbut_->attach( ensureBelow, selgrp_ );
    welltrackbut_->attach( ensureBelow, logsgrp_ );

    if ( SI().zIsTime() )
    {
	csbut_ = new uiToolButton( listgrp_, "checkshot",
			"Edit Checkshot Data", mCB(this,uiWellMan,edChckSh));
	csbut_->attach( rightOf, welltrackbut_ );
	d2tbut_ = new uiToolButton( listgrp_, "z2t", "Edit Depth/Time Model",
				    mCB(this,uiWellMan, edD2T));
	d2tbut_->attach( rightOf, csbut_ );
    }

    markerbut_ = new uiToolButton( listgrp_, "edmarkers",
			"Edit Markers", mCB(this,uiWellMan, edMarkers) );
    markerbut_->attach( rightOf, d2tbut_ ? d2tbut_ : welltrackbut_ );
    lastexternal_ = markerbut_;

    uiToolButton* logtoolbut = new uiToolButton( listgrp_, "tools",
			"Log tools", mCB(this,uiWellMan,logTools) );
    logtoolbut->attach( rightOf, markerbut_ );
    lastexternal_ = logtoolbut;

    selChg( this );
    mTriggerInstanceCreatedNotifier();
}


uiWellMan::~uiWellMan()
{
    deepErase( currdrs_ );
    deepErase( curwds_ );
}


void uiWellMan::addTool( uiButton* but )
{
    but->attach( rightOf, lastexternal_ );
    lastexternal_ = but;
}


void uiWellMan::ownSelChg()
{
    getCurrentWells();
    fillLogsFld();
    setToolButtonProperties();
}


void uiWellMan::getCurrentWells()
{
    curfnms_.erase();
    deepErase( currdrs_ );
    deepErase( curwds_ );
    curmultiids_.erase();

    if ( !curioobj_ ) return;

    const int nrsel = selGroup()->nrChosen();
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const IOObj* obj = IOM().get( selgrp_->chosenID(idx) );
	if ( !obj ) continue;

	curmultiids_ += obj->key();
	curfnms_.add( BufferString( obj->fullUserExpr( true ) ) );
	curwds_ += new Well::Data;
	currdrs_ += new Well::Reader( curfnms_[idx]->buf(), *curwds_[idx] );
	currdrs_[idx]->getInfo();
    }
}


void uiWellMan::fillLogsFld()
{
    logsfld_->setEmpty();
    if ( currdrs_.isEmpty() ) return;

    availablelognms_.erase();
    currdrs_[0]->getLogInfo( availablelognms_ );
    if ( currdrs_.size() > 1 )
    {
	for ( int idx=1; idx<currdrs_.size(); idx++ )
	{
	    BufferStringSet lognms;
	    currdrs_[idx]->getLogInfo( lognms );
	    for ( int idy=0; idy<availablelognms_.size(); )
	    {
		if ( !lognms.isPresent(availablelognms_.get(idy)) )
		    availablelognms_.removeSingle( idy );
		else
		    idy++;
	    }
	}
    }

    for ( int idx=0; idx<availablelognms_.size(); idx++)
	logsfld_->addItem( availablelognms_.get(idx) );

    logsfld_->chooseAll( false );
    checkButtons();
}


void uiWellMan::checkButtons()
{
    addlogsbut_->setSensitive( curwds_.size() == 1 );
    logSel(0);
}


#define mSetButToolTip(but,front,curwellnm,end) \
    if ( !but->sensitive() ) \
	but->setToolTip( "" ); \
    else \
    { \
	tt.setEmpty(); \
	tt.add( front ).add( " '" ).add( curwellnm ).add( "' " ).add( end ); \
	but->setToolTip( tt ); \
    }

void uiWellMan::setToolButtonProperties()
{
    if ( !curioobj_ ) return;

    BufferString tt;
    BufferString curwellnm( curioobj_->name() );
    mSetButToolTip(welltrackbut_,"Edit Well Track for",curwellnm,"");
    if ( d2tbut_ )
    {
	mSetButToolTip(d2tbut_,"Edit Depth/Time model for",curwellnm,"");
    }

    if ( csbut_ )
    {
	mSetButToolTip(csbut_,"Edit Checkshot Data for",curwellnm,"");
    }

    mSetButToolTip(markerbut_,"Edit",curwellnm," markers");

    const int nrlogs = logsfld_->size();
    const int curidx = logsfld_->currentItem();
    logdownbut_->setSensitive( curidx >= 0 && curidx < nrlogs-1 );
    logupbut_->setSensitive( curidx > 0 );

    const int nrchosen = logsfld_->nrChosen();
    const bool issing = nrchosen == 1;
    const bool oneormore = nrchosen > 0;

    logvwbut_->setSensitive( issing || nrchosen == 2 );
    logrenamebut_->setSensitive( nrlogs > 0 );
    logrmbut_->setSensitive( oneormore );
    logexpbut_->setSensitive( oneormore );
    loguombut_->setSensitive( nrlogs > 0 );

    mSetButToolTip(logupbut_,"Move",logsfld_->getText(),"up");
    mSetButToolTip(logdownbut_,"Move",logsfld_->getText(),"down");
    mSetButToolTip(logrenamebut_,"Rename",logsfld_->getText(),"");
    mSetButToolTip(loguombut_,"View/edit",logsfld_->getText(),
		   "unit of measure");
    if ( curidx < 0 )
	logvwbut_->setToolTip( "View selected log" );
    else
    {
	BufferStringSet nms;
	logsfld_->getChosen( nms );
	mSetButToolTip(logvwbut_,"View",nms.getDispString(2),"");
	mSetButToolTip(logrmbut_,"Remove",nms.getDispString(3),"");
	mSetButToolTip(logexpbut_,"Export",nms.getDispString(3),"");
    }
}


void uiWellMan::logSel( CallBacker* )
{
    setToolButtonProperties();
}


#define mErrRet(msg) \
{ uiMSG().error(msg); return; }


void uiWellMan::edMarkers( CallBacker* )
{
    if ( curwds_.isEmpty() || currdrs_.isEmpty() ) return;

    Well::Data* wd;
    if ( Well::MGR().isLoaded( curioobj_->key() ) )
	wd = Well::MGR().get( curioobj_->key() );
    else
    {
	if ( curwds_[0]->markers().isEmpty() )
	    currdrs_[0]->getMarkers();
	wd = curwds_[0];
    }

    const Well::MarkerSet origmarkers = wd->markers();

    wd->track().setName( curioobj_->name() );
    uiMarkerDlg dlg( this, wd->track() );
    dlg.setMarkerSet( wd->markers() );
    if ( !dlg.go() ) return;

    dlg.getMarkerSet( wd->markers() );
    Well::Writer wtr( curfnms_[0]->buf(), *wd );
    if ( !wtr.putMarkers() )
    {
	uiMSG().error( "Cannot write new markers to disk" );
	wd->markers() = origmarkers;
    }

    wd->markerschanged.trigger();
}


void uiWellMan::edWellTrack( CallBacker* )
{
    if ( curwds_.isEmpty() || currdrs_.isEmpty() ) return;

    Well::Data* wd;
    if ( Well::MGR().isLoaded( curioobj_->key() ) )
	wd = Well::MGR().get( curioobj_->key() );
    else
	wd = curwds_[0];

    const Well::Track origtrck = wd->track();
    const Coord origpos = wd->info().surfacecoord;
    const float origgl = wd->info().groundelev;

    uiWellTrackDlg dlg( this, *wd );
    if ( !dlg.go() ) return;

    Well::Writer wtr( curfnms_[0]->buf(), *wd );
    if ( !wtr.putInfoAndTrack( ) )
    {
	uiMSG().error( "Cannot write new track to disk" );
	wd->track() = origtrck;
	wd->info().surfacecoord = origpos;
	wd->info().groundelev = origgl;
    }

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
    if ( curwds_.isEmpty() || currdrs_.isEmpty() ) return;

    Well::Data* wd;
    if ( Well::MGR().isLoaded( curioobj_->key() ) )
	wd = Well::MGR().get( curioobj_->key() );
    else
    {
	if ( !chkshot && !curwds_[0]->d2TModel() )
	    currdrs_[0]->getD2T();
	else if ( chkshot && !curwds_[0]->checkShotModel() )
	    currdrs_[0]->getCSMdl();
	wd = curwds_[0];
    }

    if ( !chkshot && !wd->d2TModel() )
	wd->setD2TModel( new Well::D2TModel );
    if ( chkshot && !wd->checkShotModel() )
	wd->setCheckShotModel( new Well::D2TModel );

    const float oldreplvel = wd->info().replvel;
    Well::D2TModel* origd2t =
	new Well::D2TModel(*(chkshot ? wd->checkShotModel() : wd->d2TModel()));

    uiD2TModelDlg dlg( this, *wd, chkshot );
    if ( !dlg.go() ) return;

    BufferString errmsg;
    Well::Writer wtr( curfnms_[0]->buf(), *wd );
    if ( (!chkshot && !wtr.putD2T()) || (chkshot && !wtr.putCSMdl()) )
    {
	errmsg.add( "Cannot write new model to disk" );
	if ( chkshot )
	    wd->setCheckShotModel( origd2t );
	else
	{
	    wd->setD2TModel( origd2t );
	    wd->info().replvel = oldreplvel;
	}
    }
    else if ( !mIsEqual(oldreplvel,wd->info().replvel,1e-2f) &&
	      !wtr.putInfoAndTrack() )
    {
	if ( !errmsg.isEmpty() ) errmsg.addNewLine();
	errmsg.add( "Cannot write new replacement velocity to disk" );
	wd->info().replvel = oldreplvel;
    }

    if ( !errmsg.isEmpty() )
	uiMSG().error( errmsg );

    wd->d2tchanged.trigger();
    wd->trackchanged.trigger();
    mkFileInfo();
}


void uiWellMan::logTools( CallBacker* )
{
    uiWellLogToolWinMgr tooldlg( this );
    tooldlg.go();
}


#define mDeleteLogs(idx) \
    if ( !curwds_.validIdx( idx ) ) return;\
    while ( curwds_[idx]->logs().size() ) \
        delete curwds_[idx]->logs().remove(0);

void uiWellMan::importLogs( CallBacker* )
{
    uiImportLogsDlg dlg( this, curioobj_ );
    if ( dlg.go() )
	wellsChgd();
}


void uiWellMan::calcLogs( CallBacker* )
{
    if ( curwds_.isEmpty() || currdrs_.isEmpty()
	|| availablelognms_.isEmpty() || curmultiids_.isEmpty() ) return;

    currdrs_[0]->getLogs();
    uiWellLogCalc dlg( this, curmultiids_ );
    dlg.go();
    if ( dlg.haveNewLogs() )
	wellsChgd();
}


void uiWellMan::logUOMPush( CallBacker* )
{
    if ( curwds_.isEmpty() || currdrs_.isEmpty() ) return;
    const int selidx = logsfld_->currentItem();
    if ( selidx < 0 )
	mErrRet("No log selected")

    currdrs_[0]->getLogs();
    const char* lognm = logsfld_->textOfItem( selidx );
    Well::LogSet& wls = curwds_[0]->logs();
    const int curlogidx = wls.indexOf( lognm );
    if ( curlogidx < 0 )
	mErrRet( "Cannot read selected log" )

    Well::Log& wl = wls.getLog( curlogidx );
    uiWellLogUOMDlg dlg( this, wl );
    if ( !dlg.go() ) return;

    BufferString uomlbl = wl.unitMeasLabel();
    for ( int idwell=0; idwell<currdrs_.size(); idwell++ )
    {
	currdrs_[idwell]->getLogs();
	Well::Log* log = curwds_[idwell]->logs().getLog( lognm );
	if ( log ) log->setUnitMeasLabel( uomlbl );
    }
    writeLogs();
}


void uiWellMan::moveLogsPush( CallBacker* cb )
{
    if ( curwds_.isEmpty() || currdrs_.isEmpty() ) return;

    mDynamicCastGet(uiToolButton*,toolbut,cb);
    if ( toolbut != logupbut_ && toolbut != logdownbut_ )
	return;
    bool isup = toolbut == logupbut_;
    const int curlogidx = logsfld_->currentItem();
    const int newlogidx = curlogidx + ( isup ? -1 : 1 );
    Well::LogSet& wls = curwds_[0]->logs();
    currdrs_[0]->getLogs();
    if ( !wls.validIdx( curlogidx ) || !wls.validIdx( newlogidx ) )
	return;
    wls.swap( curlogidx, newlogidx );

    writeLogs();
    logsfld_->setCurrentItem( newlogidx );
}


void uiWellMan::writeLogs()
{
    for ( int idwell=0; idwell<curwds_.size(); idwell++ )
    {
	Well::Writer wtr( curfnms_[idwell]->buf(), *curwds_[idwell] );
	wtr.putLogs();
    }
    wellsChgd();
}


void uiWellMan::wellsChgd()
{
    for ( int idwell=0; idwell<curwds_.size(); idwell++ )
    {
	fillLogsFld();
	Well::MGR().reload( curmultiids_[idwell] );
	mDeleteLogs(idwell);
    }
}


#define mEnsureLogSelected() \
    if ( logsfld_->isEmpty() ) return; \
    const int nrsel = logsfld_->nrChosen(); \
    if ( nrsel < 1 ) \
	mErrRet("No log selected")

#define mGetWL() \
    currdrs_[0]->getLogs(); \
    const Well::LogSet& wls = curwds_[0]->logs(); \
    const char* lognm = logsfld_->textOfItem( logsfld_->firstChosen() ); \
    const Well::Log* wl = wls.getLog( lognm ); \
    if ( !wl ) \
	mErrRet( "Cannot read selected log" )


void uiWellMan::viewLogPush( CallBacker* )
{
    mEnsureLogSelected();
    mGetWL();

    BufferString lognm1(lognm), lognm2;
    for ( int idx=0; idx<logsfld_->size(); idx++ )
    {
	if ( !logsfld_->isChosen(idx) )
	    continue;
	const char* nm2 = logsfld_->textOfItem( idx );
	if ( lognm1 != nm2 )
	    { lognm2 = nm2; break; }
    }

    const Well::Log* wl2 = lognm2.isEmpty() ? 0 : wls.getLog( lognm2 );
    (void)uiWellLogDispDlg::popupNonModal( this, wl, wl2 );
}


void uiWellMan::renameLogPush( CallBacker* )
{
    mEnsureLogSelected();
    BufferString lognm( logsfld_->getText() );
    const BufferString titl( "Rename '", lognm, "'" );
    uiGenInputDlg dlg( this, titl, "New name", new StringInpSpec(lognm) );
    if ( !dlg.go() )
	return;

    BufferString newnm = dlg.text();
    if ( logsfld_->isPresent(newnm) )
	mErrRet("Name already in use")

    for ( int idwell=0; idwell<currdrs_.size(); idwell++ )
    {
	currdrs_[idwell]->getLogs();
	Well::Log* log = curwds_[idwell]->logs().getLog( lognm );
	if ( log ) log->setName( newnm );
    }
    writeLogs();
}


void uiWellMan::removeLogPush( CallBacker* )
{
    mEnsureLogSelected();

    BufferString msg;
    msg = nrsel  == 1 ? "This log " : "These logs ";
    msg += "will be removed from disk.\nDo you wish to continue?";
    if ( !uiMSG().askRemove(msg) )
	return;

    BufferStringSet logs2rem; logsfld_->getChosen( logs2rem );

    for ( int idwell=0; idwell<currdrs_.size(); idwell++ )
    {
	currdrs_[idwell]->getLogs();
	Well::LogSet& wls = curwds_[idwell]->logs();
	for ( int idrem=0; idrem<logs2rem.size(); idrem++ )
	{
	    BufferString logname( logs2rem.get( idrem ) );
	    const Well::Log* log = wls.getLog( logname );
	    if ( log )
		delete wls.remove( wls.indexOf( logname ) );
	}
	currdrs_[idwell]->removeAll( Well::IO::sExtLog() );
    }
    writeLogs();
}


void uiWellMan::exportLogs( CallBacker* )
{
    mEnsureLogSelected();

    BufferStringSet sellogs; logsfld_->getChosen( sellogs );

    for ( int idwell=0; idwell<currdrs_.size(); idwell++ )
    {
	currdrs_[idwell]->getLogs();
	currdrs_[idwell]->getD2T();
	const IOObj* obj = IOM().get( curmultiids_[idwell] );
	if ( obj ) curwds_[idwell]->info().setName( obj->name() );
    }

    uiExportLogs dlg( this, curwds_, sellogs );
    dlg.go();

    for ( int idw=0; idw<curwds_.size(); idw++ )
	{ mDeleteLogs(idw); currdrs_[idw]->getInfo(); }
}


void uiWellMan::mkFileInfo()
{
    if ( !curioobj_ )
    {
	setInfo( "" );
	return;
    }

    const BufferString fnm( curioobj_->fullUserExpr( true ) );
    Well::Data curwd( curioobj_->name() ) ;
    const Well::Reader currdr( fnm, curwd );
    if( !currdr.getInfo() )
    {
	setInfo( "Information not found." );
	return;
    }

    const Well::Info& info = curwd.info();
    const Well::Track& track = curwd.track();

    BufferString txt;
#define mAddWellInfo(key,str) \
    if ( !str.isEmpty() ) \
    { txt += key; txt += ": "; txt += str; txt += "\n"; }

    const BufferString posstr( info.surfacecoord.toString(), " ",
		SI().transform(info.surfacecoord).toString() );
    mAddWellInfo(Well::Info::sKeycoord(),posstr)

    if ( !track.isEmpty() )
    {
	const float rdelev = track.getKbElev();
	const UnitOfMeasure* zun = UnitOfMeasure::surveyDefDepthUnit();
	if ( !mIsZero(rdelev,1e-4) && !mIsUdf(rdelev) )
	{
	    txt += "Reference Datum Elevation (KB): ";
	    txt += zun ? zun->userValue(rdelev) : rdelev;
	    if ( zun ) txt.add( zun->symbol() );
	    txt.add( "\n" );
	}

	const float td = track.dahRange().stop;
	if ( !mIsZero(td,1e-3f) && !mIsUdf(td) )
	{
	    txt += "Total Depth (TD): ";
	    txt += zun ? zun->userValue(td) : td;
	    if ( zun ) txt.add( zun->symbol() );
	    txt.add( "\n" );
	}

	const float srdelev = info.srdelev;
	const float srd = mCast(float,SI().seismicReferenceDatum());
	if ( !mIsZero(srd,1e-4f) )
	{
	    txt.add( "Seismic Reference Datum (SRD): " );
	    txt.add( zun ? zun->userValue(srd) : srd );
	    if ( zun ) txt.add( zun->symbol() );
	    txt.add( "\n" );
	}
	if ( !mIsEqual(srd,srdelev,1e-2f) &&
	     !mIsUdf(srdelev) && !mIsZero(srdelev,1e-2f) )
	{
	    txt.add( "Warning: Seismic Reference Datum (SRD) from well " );
	    txt.add( "is no longer supported.\n" );
	    txt.add("This later is ignored, using SRD from survey settings.\n");
	}

	const float replvel = info.replvel;
	if ( !mIsUdf(replvel) )
	{
	     txt += "Replacement velocity (from KB to SRD): ";
	     txt += zun ? zun->userValue(replvel) : replvel;
	     txt += UnitOfMeasure::zUnitAnnot( false, true, false );
	     txt += "/s\n";
	}

	const float groundelev = info.groundelev;
	if ( !mIsUdf(groundelev) )
	{
	    txt += "Ground level elevation (GL): ";
	    txt += zun ? zun->userValue(groundelev) : groundelev;
	    txt += zun->symbol(); txt += "\n";
	}
    }

    mAddWellInfo(Well::Info::sKeyuwid(),info.uwid)
    mAddWellInfo(Well::Info::sKeyoper(),info.oper)
    mAddWellInfo(Well::Info::sKeystate(),info.state)
    mAddWellInfo(Well::Info::sKeycounty(),info.county)

    txt += getFileInfo();
    setInfo( txt );
}


static bool addSize( const char* basefnm, const char* fnmend,
		     double& totalsz, int& nrfiles )
{
    BufferString fnm( basefnm ); fnm += fnmend;
    if ( !File::exists(fnm) ) return false;

    totalsz += (double)File::getKbSize( fnm );
    nrfiles++;
    return true;
}


double uiWellMan::getFileSize( const char* filenm, int& nrfiles ) const
{
    if ( File::isEmpty(filenm) ) return -1;

    double totalsz = (double)File::getKbSize( filenm );
    nrfiles = 1;

    FilePath fp( filenm ); fp.setExtension( 0 );
    const BufferString basefnm( fp.fullPath() );

    addSize( basefnm, Well::IO::sExtMarkers(), totalsz, nrfiles );
    addSize( basefnm, Well::IO::sExtD2T(), totalsz, nrfiles );

    for ( int idx=1; ; idx++ )
    {
	BufferString fnmend( "^" ); fnmend += idx;
	fnmend += Well::IO::sExtLog();
	if ( !addSize(basefnm,fnmend,totalsz,nrfiles) )
	    break;
    }

    return totalsz;
}


const BufferStringSet& uiWellMan::getAvailableLogs() const
{
    return availablelognms_;
}
