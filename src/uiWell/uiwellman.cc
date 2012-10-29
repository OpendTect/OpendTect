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
#include "uiwelllogcalc.h"
#include "uiwelllogtools.h"
#include "uiwellmarkerdlg.h"

mDefineInstanceCreatedNotifierAccess(uiWellMan)


uiWellMan::uiWellMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Manage Wells",mNoDlgTitle,
				     "107.1.0").nrstatusflds(1),
	           WellTranslatorGroup::ioContext() )
{
    createDefaultUI();
    setPrefWidth( 50 );

    logsgrp_ = new uiGroup( listgrp_, "Logs group" );
    uiLabel* lbl = new uiLabel( logsgrp_, "Logs" );
    logsfld_ = new uiListBox( logsgrp_, "Available logs", true );
    logsfld_->attach( alignedBelow, lbl );

    uiButtonGroup* logsbgrp = new uiButtonGroup( listgrp_, "Logs buttons",
	    					 false );
    addlogsbut_ = new uiPushButton( logsbgrp, "&Import", false );
    addlogsbut_->activated.notify( mCB(this,uiWellMan,importLogs) );
    calclogsbut_ = new uiPushButton( logsbgrp, "&Create", false );
    calclogsbut_->activated.notify( mCB(this,uiWellMan,calcLogs) );
    calclogsbut_->attach( rightOf, addlogsbut_ );
    logsbgrp->attach( centeredBelow, logsgrp_ );

    uiManipButGrp* butgrp = new uiManipButGrp( logsgrp_ );
    butgrp->addButton( uiManipButGrp::Rename, "Rename selected log",
			mCB(this,uiWellMan,renameLogPush) );
    butgrp->addButton( uiManipButGrp::Remove, "Remove selected log",
			mCB(this,uiWellMan,removeLogPush) );
    butgrp->addButton( "export", "Export log",
	    		mCB(this,uiWellMan,exportLogs) );
    butgrp->addButton( "unitsofmeasure", "View/edit unit of measure",
	    		mCB(this,uiWellMan,logUOMPush) );
    logupbut_ = butgrp->addButton( "uparrow", "Move up",
	    		mCB(this,uiWellMan,moveLogsPush) );
    logdownbut_ = butgrp->addButton( "downarrow", "Move down",
	    		mCB(this,uiWellMan,moveLogsPush) );
    logsfld_->selectionChanged.notify( mCB(this,uiWellMan,checkMoveLogs) );
    selGroup()->getListField()->setMultiSelect(true);
    butgrp->attach( rightOf, logsfld_ );
    logsgrp_->attach( rightOf, selgrp_ );

    uiToolButton* welltrackbut = new uiToolButton( listgrp_, "edwelltrack",
	    	"Edit Well Track", mCB(this,uiWellMan, edWellTrack) );
    welltrackbut->attach( alignedBelow, selgrp_ );
    welltrackbut->attach( ensureBelow, selgrp_ );
    welltrackbut->attach( ensureBelow, logsgrp_ );
    
    uiToolButton* d2tbut = 0;
    if ( SI().zIsTime() )
    {
	uiToolButton* csbut = new uiToolButton( listgrp_, "checkshot",
			"Edit Checkshot Data", mCB(this,uiWellMan,edChckSh));
	csbut->attach( rightOf, welltrackbut );
	d2tbut = new uiToolButton( listgrp_, "z2t", "Edit Depth/Time Model",
				   mCB(this,uiWellMan, edD2T));
	d2tbut->attach( rightOf, csbut );
    }

    uiToolButton* markerbut = new uiToolButton( listgrp_, "edmarkers",
	    		"Edit Markers", mCB(this,uiWellMan, edMarkers) );
    markerbut->attach( rightOf, d2tbut ? d2tbut : welltrackbut );
    lastexternal_ = markerbut;

    uiToolButton* logtoolbut = new uiToolButton( listgrp_, "tools",
	    		"Log tools", mCB(this,uiWellMan,logTools) );
    logtoolbut->attach( rightOf, markerbut );
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
}


void uiWellMan::getCurrentWells()
{
    curfnms_.erase();
    deepErase( currdrs_ );
    deepErase( curwds_ );
    curmultiids_.erase();

    if ( !curioobj_ ) return;

    const int nrsel = selGroup()->getListField()->nrSelected();

    for ( int idx=0; idx<nrsel; idx++ )
    {
	const IOObj* obj = IOM().get( selgrp_->selected(idx) );
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

    BufferStringSet lognms;
    for ( int idx=0; idx<currdrs_.size(); idx++ )
    {
	availablelognms_.erase();
	currdrs_[idx]->getLogInfo( availablelognms_ );
	lognms.add( availablelognms_, true );
    }

    if ( currdrs_.size() > 1 )
    {
	BufferStringSet alllognms;
	while ( !lognms.isEmpty() )
	{
	    BufferString lognm = *lognms.removeSingle(0);
	    bool ispresent = true;
	    for ( int idx=0; idx<currdrs_.size(); idx++ )
	    {
		availablelognms_.erase();
		currdrs_[idx]->getLogInfo( availablelognms_ );
		if ( !availablelognms_.isPresent( lognm ) )
		    { ispresent = false; break; }
	    }
	    if ( ispresent )
		alllognms.addIfNew( lognm );
	}
	availablelognms_.erase();
	availablelognms_.add( alllognms, true );
    }

    for ( int idx=0; idx<availablelognms_.size(); idx++)
	logsfld_->addItem( availablelognms_.get(idx) );

    logsfld_->selectAll( false );
    checkButtons();
}


void uiWellMan::checkButtons()
{
    addlogsbut_->setSensitive( curwds_.size() == 1 );
    checkMoveLogs(0);
}


void uiWellMan::checkMoveLogs( CallBacker* )
{
    const int curidx = logsfld_->currentItem();
    const int nrlogs = logsfld_->size();
    const int nrlogsel = logsfld_->nrSelected();
    const int nrsel_ = selGroup()->getListField()->nrSelected();
    bool nomove = ( nrsel_ != 1 ) 
			&& ( nrlogsel != 1 || curidx < 0 || curidx >= nrlogs );
    bool canmoveup = curidx > 0 && !nomove && curwds_.size() == 1;
    bool canmovedown = curidx < nrlogs-1 && !nomove && curwds_.size() == 1;
    logupbut_->setSensitive( canmoveup );
    logdownbut_->setSensitive( canmovedown );
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

    wd->track().setName( curioobj_->name() );
    uiMarkerDlg dlg( this, wd->track() );
    dlg.setMarkerSet( wd->markers() );
    if ( !dlg.go() ) return;

    dlg.getMarkerSet( wd->markers() );
    Well::Writer wtr( curfnms_[0]->buf(), *wd );
    if ( !wtr.putMarkers() ) 
	uiMSG().error( "Cannot write new markers to disk" );

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

    uiWellTrackDlg dlg( this, *wd );
    if ( !dlg.go() ) return;

    Well::Writer wtr( curfnms_[0]->buf(), *wd );
    if ( !wtr.putInfoAndTrack( ) )
	uiMSG().error( "Cannot write new track to disk" );

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

    uiD2TModelDlg dlg( this, *wd, chkshot );
    if ( !dlg.go() ) return;
    Well::Writer wtr( curfnms_[0]->buf(), *wd );
    if ( (!chkshot && !wtr.putD2T()) || (chkshot && !wtr.putCSMdl()) )
	uiMSG().error( "Cannot write new model to disk" );
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
    if ( curwds_.isEmpty() || currdrs_.isEmpty() ) return;

    currdrs_[0]->getLogs();
    uiLoadLogsDlg dlg( this, *curwds_[0] );
    if ( dlg.go() )
	writeLogs();
}


void uiWellMan::calcLogs( CallBacker* )
{
    if ( curwds_.isEmpty() || currdrs_.isEmpty()
	|| availablelognms_.isEmpty() || curmultiids_.isEmpty() ) return;

    currdrs_[0]->getLogs();
    uiWellLogCalc dlg( this, curwds_[0]->logs(), availablelognms_,
	    		curmultiids_ );
    dlg.go();
    if ( dlg.haveNewLogs() )
	wellsChgd();
}


void uiWellMan::logUOMPush( CallBacker* )
{
    if ( curwds_.isEmpty() || currdrs_.isEmpty() ) return;
    if ( !logsfld_->size() || !logsfld_->nrSelected() )
	mErrRet("No log selected")

    currdrs_[0]->getLogs();
    const char* lognm = logsfld_->getText();
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


void uiWellMan::exportLogs( CallBacker* )
{
    if ( !logsfld_->size() || !logsfld_->nrSelected() ) return;

    BufferStringSet sellogs;
    logsfld_->getSelectedItems( sellogs );

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


void uiWellMan::removeLogPush( CallBacker* )
{
    if ( !logsfld_->size() || !logsfld_->nrSelected() ) return;

    BufferString msg;
    msg = logsfld_->nrSelected() == 1 ? "This log " : "These logs ";
    msg += "will be removed from disk.\nDo you wish to continue?";
    if ( !uiMSG().askRemove(msg) )
	return;

    BufferStringSet logs2rem;
    logsfld_->getSelectedItems( logs2rem );

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


void uiWellMan::renameLogPush( CallBacker* )
{
    if ( !logsfld_->size() || !logsfld_->nrSelected() )
	mErrRet("No log selected")

    currdrs_[0]->getLogs();
    Well::LogSet& wls = curwds_[0]->logs();
    const char* lognm = logsfld_->getText();
    if ( !wls.getLog( lognm ) ) 
	mErrRet( "Cannot read selected log" )

    BufferString titl( "Rename '" );
    titl += lognm; titl += "'";
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


void uiWellMan::mkFileInfo()
{
    if ( curwds_.isEmpty() || curwds_.size() > 1 
	    || currdrs_.isEmpty() || currdrs_.size() > 1 || !curioobj_ )
    {
	setInfo( "" );
	return;
    }

    BufferString txt;

#define mAddWellInfo(key,str) \
    if ( !str.isEmpty() ) \
    { txt += key; txt += ": "; txt += str; txt += "\n"; }

    const Well::Info& info = curwds_[0]->info();
    const Well::Track& track = curwds_[0]->track();

    BufferString crdstr; info.surfacecoord.fill( crdstr.buf() );
    BufferString bidstr; SI().transform(info.surfacecoord).fill( bidstr.buf() );
    BufferString posstr( bidstr ); posstr += " "; posstr += crdstr;
    mAddWellInfo(Well::Info::sKeycoord(),posstr)

    if ( !track.isEmpty() )
    {
	const float rdelev = track.dah(0) - track.value(0);
	const UnitOfMeasure* zun = UnitOfMeasure::surveyDefDepthUnit();
	if ( !mIsZero(rdelev,1e-4) && !mIsUdf(rdelev) )
	{
	    txt += "Reference Datum Elevation (KB)"; txt += ": ";
	    txt += zun ? zun->userValue(rdelev) : rdelev;
	    txt += zun->symbol(); txt += "\n";
	}

	const float surfelev = -info.surfaceelev;
	if ( !mIsZero(surfelev,1e-4) && !mIsUdf(surfelev) )
	{
	    txt += "Seismic Reference Datum (SRD)"; txt += ": ";
	    txt += zun ? zun->userValue(surfelev) : surfelev;
	    txt += zun->symbol(); txt += "\n";
	}

	const float replvel = info.replvel;
	if ( !mIsUdf(replvel) )
	{
	     txt += "Replacement velocity (from KB to SRD)"; txt += ": ";
	     txt += zun ? zun->userValue(replvel) : replvel;
	     txt += UnitOfMeasure::zUnitAnnot( false, true, false );
	     txt += "/s\n";
	}

	const float groundelev = info.groundelev;
	if ( !mIsUdf(groundelev) )
	{
	    txt += "Ground level elevation (GL)"; txt += ": ";
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


void uiWellMan::getSelLogs( BufferStringSet& lognms ) const
{ logsfld_->getSelectedItems( lognms ); }


const BufferStringSet& uiWellMan::getAvailableLogs() const
{ return availablelognms_; }
