/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellman.cc,v 1.74 2010-12-16 13:04:30 cvsbert Exp $";

#include "uiwellman.h"

#include "bufstringset.h"
#include "ioobj.h"
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
#include "uiwellmarkerdlg.h"


Notifier<uiWellMan>* uiWellMan::fieldsCreated()
{
    static Notifier<uiWellMan> FieldsCreated(0);
    return &FieldsCreated;
}


uiWellMan::uiWellMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Well file management","Manage wells",
				     "107.1.0").nrstatusflds(1),
	           WellTranslatorGroup::ioContext() )
    , curwd_(0)
    , currdr_(0)
    , curfnm_("")
{
    createDefaultUI();
    setPrefWidth( 50 );

    logsgrp_ = new uiGroup( listgrp_, "Logs group" );
    uiLabel* lbl = new uiLabel( logsgrp_, "Logs" );
    logsfld_ = new uiListBox( logsgrp_, "Available logs", true );
    logsfld_->attach( alignedBelow, lbl );

    uiButtonGroup* logsbgrp = new uiButtonGroup( listgrp_, "Logs buttons",
	    					 false );
    uiPushButton* addlogsbut = new uiPushButton( logsbgrp, "&Import", false );
    addlogsbut->activated.notify( mCB(this,uiWellMan,importLogs) );
    uiPushButton* calclogsbut = new uiPushButton( logsbgrp, "&Create", false );
    calclogsbut->activated.notify( mCB(this,uiWellMan,calcLogs) );
    calclogsbut->attach( rightOf, addlogsbut );
    logsbgrp->attach( centeredBelow, logsgrp_ );

    uiManipButGrp* butgrp = new uiManipButGrp( logsgrp_ );
    butgrp->addButton( uiManipButGrp::Rename, "Rename selected log",
			mCB(this,uiWellMan,renameLogPush) );
    butgrp->addButton( uiManipButGrp::Remove, "Remove selected log",
			mCB(this,uiWellMan,removeLogPush) );
    butgrp->addButton( "export.png", "Export log",
	    		mCB(this,uiWellMan,exportLogs) );
    butgrp->attach( rightOf, logsfld_ );
    logsgrp_->attach( rightOf, selgrp_ );

    uiToolButton* welltrackbut = new uiToolButton( listgrp_, "edwelltrack.png",
	    	"Edit Well Track", mCB(this,uiWellMan, edWellTrack) );
    welltrackbut->attach( alignedBelow, selgrp_ );
    welltrackbut->attach( ensureBelow, selgrp_ );
    welltrackbut->attach( ensureBelow, logsgrp_ );
    
    uiToolButton* d2tbut = 0;
    if ( SI().zIsTime() )
    {
	uiToolButton* csbut = new uiToolButton( listgrp_, "checkshot.png",
			"Edit Checkshot Data", mCB(this,uiWellMan,edChckSh));
	csbut->attach( rightOf, welltrackbut );
	d2tbut = new uiToolButton( listgrp_, "z2t.png", "Edit Depth/Time Model",
				   mCB(this,uiWellMan, edD2T));
	d2tbut->attach( rightOf, csbut );
    }

    uiToolButton* markerbut = new uiToolButton( listgrp_, "edmarkers.png",
	    		"Edit Markers", mCB(this,uiWellMan, edMarkers) );
    markerbut->attach( rightOf, d2tbut ? d2tbut : welltrackbut );
    lastexternal_ = markerbut;

    selChg( this );
    fieldsCreated()->trigger( this );
}


uiWellMan::~uiWellMan()
{
    delete curwd_;
    delete currdr_;
}


void uiWellMan::addTool( uiButton* but )
{
    but->attach( rightOf, lastexternal_ );
    lastexternal_ = but;
}


void uiWellMan::ownSelChg()
{
    getCurrentWell();
    fillLogsFld();
}


void uiWellMan::getCurrentWell()
{
    curfnm_ = ""; 
    delete currdr_; currdr_ = 0;
    delete curwd_; curwd_ = 0;
    if ( !curioobj_ ) return;
    
    curfnm_ = curioobj_->fullUserExpr( true );
    curwd_ = new Well::Data;
    currdr_ = new Well::Reader( curfnm_, *curwd_ );
    currdr_->getInfo();
}


void uiWellMan::fillLogsFld()
{
    logsfld_->setEmpty();
    if ( !currdr_ ) return;

    BufferStringSet lognms;
    currdr_->getLogInfo( lognms );
    for ( int idx=0; idx<lognms.size(); idx++)
	logsfld_->addItem( lognms.get(idx) );
    logsfld_->selectAll( false );
}


#define mErrRet(msg) \
{ uiMSG().error(msg); return; }


void uiWellMan::edMarkers( CallBacker* )
{
    if ( !curwd_ || !currdr_ ) return;

    Well::Data* wd;
    if ( Well::MGR().isLoaded( curioobj_->key() ) )
	wd = Well::MGR().get( curioobj_->key() );
    else
    {
	if ( curwd_->markers().isEmpty() )
	    currdr_->getMarkers();
	wd = curwd_;
    }


    uiMarkerDlg dlg( this, wd->track() );
    dlg.setMarkerSet( wd->markers() );
    if ( !dlg.go() ) return;

    dlg.getMarkerSet( wd->markers() );
    Well::Writer wtr( curfnm_, *wd );
    if ( !wtr.putMarkers() ) 
	uiMSG().error( "Cannot write new markers to disk" );

    wd->markerschanged.trigger();
}


void uiWellMan::edWellTrack( CallBacker* )
{
    if ( !curwd_ || !currdr_ ) return;

    Well::Data* wd;
    if ( Well::MGR().isLoaded( curioobj_->key() ) )
	wd = Well::MGR().get( curioobj_->key() );
    else
	wd = curwd_;

    uiWellTrackDlg dlg( this, *wd );
    if ( !dlg.go() ) return;

    Well::Writer wtr( curfnm_, *wd );
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
    if ( !curwd_ || !currdr_ ) return;

    Well::Data* wd;
    if ( Well::MGR().isLoaded( curioobj_->key() ) )
	wd = Well::MGR().get( curioobj_->key() );
    else
    {
	if ( !chkshot && !curwd_->d2TModel() )
	    currdr_->getD2T();
	else if ( chkshot && !curwd_->checkShotModel() )
	    currdr_->getCSMdl();
	wd = curwd_;
    }

    if ( !chkshot && !wd->d2TModel() )
	wd->setD2TModel( new Well::D2TModel );
    if ( chkshot && !wd->checkShotModel() )
	wd->setCheckShotModel( new Well::D2TModel );

    uiD2TModelDlg dlg( this, *wd, chkshot );
    if ( !dlg.go() ) return;
    Well::Writer wtr( curfnm_, *wd );
    if ( (!chkshot && !wtr.putD2T()) || (chkshot && !wtr.putCSMdl()) )
	uiMSG().error( "Cannot write new model to disk" );
}


#define mDeleteLogs() \
    while ( curwd_->logs().size() ) \
        delete curwd_->logs().remove(0);

void uiWellMan::importLogs( CallBacker* )
{
    if ( !curwd_ || !currdr_ ) return;

    currdr_->getLogs();
    uiLoadLogsDlg dlg( this, *curwd_ );
    if ( dlg.go() )
	writeLogs();
}


void uiWellMan::calcLogs( CallBacker* )
{
    if ( !curwd_ || !currdr_ ) return;

    currdr_->getLogs();
    uiWellLogCalc dlg( this, curwd_->logs() );
    dlg.go();
    if ( dlg.haveNewLogs() )
	writeLogs();
}


void uiWellMan::writeLogs()
{
    Well::Writer wtr( curfnm_, *curwd_ );
    wtr.putLogs();

    fillLogsFld();
    const MultiID& key = curioobj_->key();
    Well::MGR().reload( key );

    mDeleteLogs();
}


void uiWellMan::exportLogs( CallBacker* )
{
    if ( !logsfld_->size() || !logsfld_->nrSelected() ) return;

    BoolTypeSet issel;
    for ( int idx=0; idx<logsfld_->size(); idx++ )
	issel += logsfld_->isSelected(idx);

    if ( !curwd_->d2TModel() )
	currdr_->getD2T();

    currdr_->getLogs();
    uiExportLogs dlg( this, *curwd_, issel );
    dlg.go();

    mDeleteLogs();
}


void uiWellMan::removeLogPush( CallBacker* )
{
    if ( !logsfld_->size() || !logsfld_->nrSelected() ) return;

    BufferString msg;
    msg = logsfld_->nrSelected() == 1 ? "This log " : "These logs ";
    msg += "will be removed from disk.\nDo you wish to continue?";
    if ( !uiMSG().askRemove(msg) )
	return;

    currdr_->getLogs();
    Well::LogSet& wls = curwd_->logs();
    BufferStringSet logs2rem;
    for ( int idx=0; idx<logsfld_->size(); idx++ )
    {
	if ( logsfld_->isSelected(idx) )
	    logs2rem.add( wls.getLog(idx).name() );
    }

    for ( int idx=0; idx<logs2rem.size(); idx++ )
    {
	BufferString& logname = logs2rem.get( idx );
	for ( int logidx=0; logidx<wls.size(); logidx++ )
	{
	    if ( logname == wls.getLog(logidx).name() )
	    {
		Well::Log* log = wls.remove( logidx );
		delete log;
		break;
	    }
	}
    }

    logs2rem.erase();

    if ( currdr_->removeAll(Well::IO::sExtLog()) )
    {
	Well::Writer wtr( curfnm_, *curwd_ );
	wtr.putLogs();
	fillLogsFld();
    }

    const MultiID& key = curioobj_->key();
    Well::MGR().reload( key );

    mDeleteLogs();
}


void uiWellMan::renameLogPush( CallBacker* )
{
    if ( !logsfld_->size() || !logsfld_->nrSelected() )
	mErrRet("No log selected")

    const int lognr = logsfld_->currentItem() + 1;
    FilePath fp( curfnm_ ); fp.setExtension( 0 );
    BufferString logfnm = Well::IO::mkFileName( fp.fullPath(),
	    					Well::IO::sExtLog(), lognr );
    StreamProvider sp( logfnm );
    StreamData sdi = sp.makeIStream();
    bool res = currdr_->addLog( *sdi.istrm );
    sdi.close();
    if ( !res ) 
	mErrRet("Cannot read selected log")

    Well::Log* log = curwd_->logs().remove( 0 );

    BufferString titl( "Rename '" );
    titl += log->name(); titl += "'";
    uiGenInputDlg dlg( this, titl, "New name",
    			new StringInpSpec(log->name()) );
    if ( !dlg.go() ) return;

    BufferString newnm = dlg.text();
    if ( logsfld_->isPresent(newnm) )
	mErrRet("Name already in use")

    log->setName( newnm );
    Well::Writer wtr( curfnm_, *curwd_ );
    StreamData sdo = sp.makeOStream();
    wtr.putLog( *sdo.ostrm, *log );
    sdo.close();
    fillLogsFld();
    const MultiID& key = curioobj_->key();
    Well::MGR().reload( key );
    delete log;
}


void uiWellMan::mkFileInfo()
{
    if ( !curwd_ || !currdr_ || !curioobj_ )
    {
	setInfo( "" );
	return;
    }

    BufferString txt;

#define mAddWellInfo(key,str) \
    if ( !str.isEmpty() ) \
    { txt += key; txt += ": "; txt += str; txt += "\n"; }

    const Well::Info& info = curwd_->info();
    const Well::Track& track = curwd_->track();

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
	    txt += zun->userValue(rdelev); txt += zun->symbol(); txt += "\n";
	}

	const float surfelev = -info.surfaceelev;
	if ( !mIsZero(surfelev,1e-4) && !mIsUdf(surfelev) )
	{
	    txt += "Surface Reference Datum"; txt += ": ";
	    txt += zun->userValue(surfelev); txt += zun->symbol(); txt += "\n";
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
