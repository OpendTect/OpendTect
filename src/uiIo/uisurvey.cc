/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisurvey.h"

#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uiconvpos.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uifont.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilatlong2coord.h"
#include "uilistbox.h"
#include "uichecklist.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisetdatadir.h"
#include "ui2dsip.h"
#include "uisip.h"
#include "uisplitter.h"
#include "uisurveyselect.h"
#include "uisurvinfoed.h"
#include "uisurvmap.h"
#include "uisurveyzip.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "angles.h"
#include "survinfo.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "dirlist.h"
#include "envvars.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "iostrm.h"
#include "latlong.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "odver.h"
#include "settings.h"
#include "od_ostream.h"
#include "od_helpids.h"


static const char*	sZipFileMask = "ZIP files (*.zip *.ZIP)";
#define mErrRetVoid(s)	{ if ( s ) uiMSG().error(s); return; }
#define mErrRet(s)	{ if ( s ) uiMSG().error(s); return false; }

static int sMapWidth = 300;
static int sMapHeight = 300;

//--- General tools


static ObjectSet<uiSurvey::Util>& getUtils()
{
    mDefineStaticLocalObject( PtrMan<ManagedObjectSet<uiSurvey::Util> >,
			      utils, = 0 );
    if ( !utils )
    {
	ManagedObjectSet<uiSurvey::Util>* newutils =
				    new ManagedObjectSet<uiSurvey::Util>;
	*newutils += new uiSurvey::Util( "xy2ic",
		"Convert (X,Y) to/from (In-line,Cross-line)", CallBack() );
	*newutils += new uiSurvey::Util( "spherewire",
				"Setup geographical coordinates",
				      CallBack() );

	if ( !utils.setIfNull(newutils) )
	    delete newutils;

    }
    return *utils;
}


static BufferString getTrueDir( const char* dn )
{
    BufferString dirnm = dn;
    FilePath fp;
    while ( File::isLink(dirnm) )
    {
	BufferString newdirnm = File::linkTarget(dirnm);
	fp.set( newdirnm );
	if ( !fp.isAbsolute() )
	{
	    FilePath dirnmfp( dirnm );
	    dirnmfp.setFileName( 0 );
	    fp.setPath( dirnmfp.fullPath() );
	}
	dirnm = fp.fullPath();
    }
    return dirnm;
}


//--- uiNewSurveyByCopy


class uiNewSurveyByCopy : public uiDialog
{

public:

uiNewSurveyByCopy( uiParent* p, const char* dataroot, const char* dirnm )
	: uiDialog(p,uiDialog::Setup("Copy survey",mNoDlgTitle,mTODOHelpKey))
	, dataroot_(dataroot)
{
    BufferString curfnm;
    if ( dirnm && *dirnm )
	curfnm = FilePath( dataroot_, dirnm ).fullPath();
    else
	curfnm = dataroot_;

    inpsurveyfld_ = new uiSurveySelect( this, true, false, "Survey to copy" );
    inpsurveyfld_->setSurveyPath( curfnm );
    newsurveyfld_ = new uiSurveySelect( this, false, false, "New Survey name" );
    newsurveyfld_->attach( alignedBelow,  inpsurveyfld_ );
}

void inpSel( CallBacker* )
{
    BufferString fullpath;
    inpsurveyfld_->getFullSurveyPath( fullpath );
    FilePath fp( fullpath );
    newsurveyfld_->setInputText( fp.fullPath() );
}

bool copySurv()
{
    if ( File::exists(newdirnm_) )
    {
        uiMSG().error( BufferString( "A survey '", newdirnm_,
		    "' already exists.\nYou will have to remove it first" ) );
        return false;
    }

    uiTaskRunner taskrunner( this );
    const BufferString fromdir = getTrueDir( inpdirnm_ );
    PtrMan<Executor> copier = File::getRecursiveCopier( fromdir, newdirnm_ );
    if ( !taskrunner.execute(*copier) )
	{ uiMSG().error( "Cannot copy the survey data" ); return false; }

    File::makeWritable( newdirnm_, true, true );
    return true;
}

bool acceptOK( CallBacker* )
{
    if ( !inpsurveyfld_->getFullSurveyPath( inpdirnm_ ) ||
	 !newsurveyfld_->getFullSurveyPath( newdirnm_) )
	mErrRet( "No Valid or Empty Input" )

    return copySurv();
}

    const BufferString	dataroot_;
    BufferString	inpdirnm_;
    BufferString	newdirnm_;
    uiSurveySelect*	inpsurveyfld_;
    uiSurveySelect*	newsurveyfld_;

};


//--- uiStartNewSurveySetup


class uiStartNewSurveySetup : public uiDialog
{

public:
			uiStartNewSurveySetup(uiParent*,const char*,
					      SurveyInfo&);

    bool		isOK();
    bool		acceptOK(CallBacker*);

protected:

    const BufferString	dataroot_;
    SurveyInfo&		survinfo_;
    uiGenInput*		survnmfld_;
    uiGenInput*		zistimefld_;
    uiGenInput*		zinfeetfld_;
    uiCheckList*	pol2dfld_;
    uiComboBox*		sipfld_;
    ObjectSet<uiSurvInfoProvider>& sips_;

    BufferString	sipName() const;
    BufferString	survName() const { return survnmfld_->text(); }
    bool		has3D() const	 { return pol2dfld_->isChecked(0); }
    bool		has2D() const	 { return pol2dfld_->isChecked(1); }
    bool		isTime() const	 { return zistimefld_->getBoolValue();}
    bool		isInFeet() const { return zinfeetfld_->getBoolValue();}

    void		setSip(bool for2donly);

SurveyInfo::Pol2D pol2D() const
{
    return has3D() ? ( has2D() ? SurveyInfo::Both2DAnd3D
			       : SurveyInfo::No2D )
			       : SurveyInfo::Only2D;
}

void pol2dChg( CallBacker* cb )
{
    setSip( has2D() && !has3D() );
}

void zdomainChg( CallBacker* cb )
{
    zinfeetfld_->display( !isTime() );
}

};


uiStartNewSurveySetup::uiStartNewSurveySetup(uiParent* p, const char* dataroot,
					      SurveyInfo& survinfo )
	: uiDialog(p,uiDialog::Setup("Create New Survey",
		    "Specify new survey parameters",mTODOHelpKey))
	, survinfo_(survinfo)
	, dataroot_(dataroot)
	, sips_(uiSurveyInfoEditor::survInfoProvs())
{
    setOkText( uiStrings::sNext() );

    survnmfld_ = new uiGenInput( this, "Survey name" );
    survnmfld_->setElemSzPol( uiObject::Wide );

    pol2dfld_ = new uiCheckList( this, uiCheckList::OneMinimum, OD::Horizontal);
    pol2dfld_->setLabel( "Available data" );
    pol2dfld_->addItem( uiStrings::s3D() ).addItem( uiStrings::s2D() );
    pol2dfld_->setChecked( 0, true ).setChecked( 1, true );
    pol2dfld_->changed.notify( mCB(this,uiStartNewSurveySetup,pol2dChg) );
    pol2dfld_->attach( alignedBelow, survnmfld_ );

    uiLabeledComboBox* siplcb = new uiLabeledComboBox( this, "Define by" );
    siplcb->attach( alignedBelow, pol2dfld_ );
    sipfld_ = siplcb->box();

    zistimefld_ = new uiGenInput( this, "Z Domain",
				  BoolInpSpec(true,uiStrings::sTime(),
                                              uiStrings::sDepth()));
    zistimefld_->valuechanged.notify(
			mCB(this,uiStartNewSurveySetup,zdomainChg) );
    zistimefld_->attach( alignedBelow, siplcb );

    zinfeetfld_ = new uiGenInput( this, "Depth unit",
				BoolInpSpec(true,"Meter","Feet") );
    zinfeetfld_->attach( alignedBelow, zistimefld_ );
    zinfeetfld_->display( false );

    setSip( false );
}


bool uiStartNewSurveySetup::isOK()
{
    BufferString survnm = survName();
    if ( survnm.isEmpty() )
	mErrRet( "Please enter a new survey name" )

    survnm.clean( BufferString::AllowDots );
    const BufferString storagedir = FilePath(dataroot_).add(survnm).fullPath();
    if ( File::exists(storagedir) )
    {
	BufferString errmsg( "A survey called ", survnm, " already exists\n" );
	errmsg.add( "Please remove it first or use another survey name" );
	mErrRet( errmsg )
    }

    const int sipidx = sipfld_->currentItem();
    if ( sipidx == sipfld_->size()-1 )
	return true; // Manual selection.

    if ( !sips_.validIdx(sipidx) )
    {
	pErrMsg( "Cannot use this geometry provider method" );
	return false;
    }

    uiSurvInfoProvider* sip = sips_[sipidx];
    if ( !sip )
	mErrRet( "Cannot use this geometry provider method" )

    return true;
}


bool uiStartNewSurveySetup::acceptOK( CallBacker* cb )
{
    if ( !isOK() )
	return false;

    const BufferString survnm = survName();
    survinfo_.setName( survnm );
    survinfo_.updateDirName();
    survinfo_.setSurvDataType( pol2D() );
    survinfo_.setZUnit( isTime(), isInFeet() );

    const int sipidx = sipfld_->currentItem();
    if ( sipidx < sipfld_->size() - 1 )
	survinfo_.setSipName( sipName() );

    return true;
}


BufferString uiStartNewSurveySetup::sipName() const
{
    const int sipidx = sipfld_->currentItem();
    return sipidx == sipfld_->size()-1 ? "" : sipfld_->textOfItem( sipidx );
}


void uiStartNewSurveySetup::setSip( bool for2donly )
{
    sipfld_->setEmpty();

    const int nrprovs = sips_.size();
    int preferedsel = nrprovs ? -1 : 0;
    for ( int idx=0; idx<nrprovs; idx++ )
    {
	if ( !sips_.validIdx(idx) )
	    continue;

	mDynamicCastGet(const ui2DSurvInfoProvider*,sip,sips_[idx]);
	if ( sip && for2donly && preferedsel == -1 )
	    preferedsel = idx;

	BufferString txt( sips_[idx]->usrText() );
	txt += " ...";
	sipfld_->addItem( txt );
    }
    sipfld_->addItem( "Manual selection" ); // always last
    sipfld_->setCurrentItem( preferedsel );

    int maxlen = 0;
    for ( int idx=0; idx<sipfld_->size(); idx++ )
    {
	const int len = FixedString( sipfld_->textOfItem(idx) ).size();
	if ( len > maxlen ) maxlen = len;
    }
    sipfld_->setPrefWidthInChar( maxlen + 5 );
}


//--- uiSurvey


uiSurvey::uiSurvey( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Survey Setup and Selection"),
				 mNoDlgTitle,mODHelpKey(mSurveyHelpID)))
    , orgdataroot_(GetBaseDataDir())
    , dataroot_(GetBaseDataDir())
    , initialsurveyname_(GetSurveyName())
    , cursurvinfo_(0)
    , survmap_(0)
    , dirfld_(0)
    , impiop_(0)
    , impsip_(0)
    , parschanged_(false)
    , cursurvremoved_(false)
{
    const CallBack selchgcb( mCB(this,uiSurvey,selChange) );

    if ( dataroot_.isEmpty() )
    {
	new uiLabel( this,
		tr("Cannot establish a 'Survey Data Root' directory."
		"\nOpendTect needs a place to store its files."
		"\nPlease consult the documentation at opendtect.org,"
		"\nor contact support@opendtect.org.") );
    }

    setCurrentSurvInfo( new SurveyInfo(SI()) );

    mDefineStaticLocalObject( int, sipidx2d, mUnusedVar =
	    uiSurveyInfoEditor::addInfoProvider(new ui2DSurvInfoProvider) );
    mDefineStaticLocalObject( int, sipidxcp, mUnusedVar =
	    uiSurveyInfoEditor::addInfoProvider(new uiCopySurveySIP) );

    uiGroup* topgrp = new uiGroup( this, "TopGroup" );
    uiPushButton* datarootbut = new uiPushButton( topgrp,
	tr("Survey Data Root"), false );
    datarootbut->activated.notify( mCB(this,uiSurvey,dataRootPushed) );
    datarootbut->attach( leftBorder );

    datarootlbl_ = new uiLabel( topgrp, "" );
    datarootlbl_->setHSzPol( uiObject::WideMax );
    datarootlbl_->attach( rightTo, datarootbut );

    uiSeparator* sep1 = new uiSeparator( topgrp, "Separator 1" );
    sep1->attach( stretchedBelow, datarootbut );

    uiGroup* leftgrp = new uiGroup( topgrp, "Survey selection left" );
    uiGroup* rightgrp = new uiGroup( topgrp, "Survey selection right" );

    fillLeftGroup( leftgrp );
    fillRightGroup( rightgrp );
    leftgrp->attach( ensureBelow, sep1 );
    rightgrp->attach( rightOf, leftgrp );

    uiLabel* infolbl = new uiLabel( topgrp, "" );
    infolbl->setPixmap( "info" );
    infolbl->setToolTip( tr("Survey Information") );
    infolbl->attach( alignedBelow, leftgrp );
    infofld_ = new uiTextEdit( topgrp, "Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setStretch( 2, 1 );
    infofld_->attach( rightTo, infolbl );
    infofld_->attach( ensureBelow, rightgrp );

    uiGroup* botgrp = new uiGroup( this, "Bottom Group" );
    uiLabel* notelbl = new uiLabel( botgrp, "" );
    notelbl->setPixmap( ioPixmap("notes") );
    notelbl->setToolTip( tr("Notes") );
    notelbl->setMaximumWidth( 32 );

    notesfld_ = new uiTextEdit( botgrp, "Survey Notes" );
    notesfld_->attach( rightTo, notelbl );
    notesfld_->setPrefHeightInChar( 6 );
    notesfld_->setStretch( 2, 2 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    putToScreen();
    updateDataRootLabel();
    setOkText( uiStrings::sSelect() );
    postFinalise().notify( selchgcb );
}


uiSurvey::~uiSurvey()
{
    delete impiop_;
    delete cursurvinfo_;
}


static void osrbuttonCB( void* )
{
    uiDesktopServices::openUrl( "https://opendtect.org/osr" );
}


void uiSurvey::fillLeftGroup( uiGroup* grp )
{
    dirfld_ = new uiListBox( grp, "Surveys" );
    updateSurvList();
    dirfld_->selectionChanged.notify( mCB(this,uiSurvey,selChange) );
    dirfld_->doubleClicked.notify( mCB(this,uiSurvey,accept) );
    dirfld_->setHSzPol( uiObject::WideVar );
    dirfld_->setStretch( 2, 2 );

    uiButtonGroup* butgrp =
	new uiButtonGroup( grp, "Buttons", OD::Vertical );
    butgrp->attach( rightTo, dirfld_ );
    new uiToolButton( butgrp, "addnew",
	"Create New Survey", mCB(this,uiSurvey,newButPushed) );
    editbut_ = new uiToolButton( butgrp, "edit", tr("Edit Survey Parameters"),
				 mCB(this,uiSurvey,editButPushed) );
    new uiToolButton( butgrp, "copyobj",
	tr("Copy Survey"), mCB(this,uiSurvey,copyButPushed) );
    new uiToolButton( butgrp, "export",
	tr("Compress survey as zip archive"),
        mCB(this,uiSurvey,exportButPushed) );
    new uiToolButton( butgrp, "import",
	tr("Extract survey from zip archive"),
        mCB(this,uiSurvey,importButPushed) );
    new uiToolButton( butgrp, "share",
	tr("Share surveys through the OpendTect Seismic Repository"),
	 mSCB(osrbuttonCB) );
    rmbut_ = new uiToolButton( butgrp, "trashcan", tr("Remove Survey"),
			       mCB(this,uiSurvey,rmButPushed) );
}


void uiSurvey::fillRightGroup( uiGroup* grp )
{
    survmap_ = new uiSurveyMap( grp );
    survmap_->setPrefWidth( sMapWidth );
    survmap_->setPrefHeight( sMapHeight );

    uiButton* lastbut = 0;
    ObjectSet<uiSurvey::Util>& utils = getUtils();
    const CallBack cb( mCB(this,uiSurvey,utilButPush) );
    for ( int idx=0; idx<utils.size(); idx++ )
    {
	const uiSurvey::Util& util = *utils[idx];
	uiToolButton* but = new uiToolButton( grp, util.pixmap_,
					      util.tooltip_, cb );
	but->setToolTip( util.tooltip_ );
	utilbuts_ += but;
	if ( !lastbut )
	    but->attach( rightTo, survmap_ );
	else
	    but->attach( alignedBelow, lastbut );
	lastbut = but;
    }
}


void uiSurvey::add( const uiSurvey::Util& util )
{
    getUtils() += new uiSurvey::Util( util );
}


const char* uiSurvey::selectedSurveyName() const
{
    return dirfld_->getText();
}


bool uiSurvey::rootDirWritable() const
{
    if ( !File::isWritable(dataroot_) )
    {
	BufferString msg( "Cannot create new survey in\n",dataroot_,
			  ".\nDirectory is write protected.");
	uiMSG().error( msg );
	return false;
    }
    return true;
}


void uiSurvey::getSurveyList( BufferStringSet& list, const char* dataroot,
				const char* excludenm )
{
    BufferString basedir = dataroot;
    if ( basedir.isEmpty() )
	basedir = GetBaseDataDir();
    DirList dl( basedir, DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString& dirnm = dl.get( idx );
	if ( excludenm && dirnm == excludenm )
	    continue;

	const FilePath fp( basedir, dirnm, SurveyInfo::sKeySetupFileName() );
	if ( File::exists(fp.fullPath()) )
	    list.add( dirnm );
    }

    list.sort();
}


bool uiSurvey::survTypeOKForUser( bool is2d )
{
    const bool dowarn = (is2d && !SI().has2D()) || (!is2d && !SI().has3D());
    if ( !dowarn ) return true;

    BufferString warnmsg( "Your survey is set up as '" );
    warnmsg += is2d ? "3-D only'.\nTo be able to actually use 2-D"
		    : "2-D only'.\nTo be able to actually use 3-D";
    warnmsg += " data\nyou will have to change the survey setup.";
    warnmsg += "\n\nDo you wish to continue?";

    return uiMSG().askContinue( warnmsg );
}


void uiSurvey::updateDataRootInSettings()
{
    Settings::common().set( "Default DATA directory", dataroot_ );
    if ( !Settings::common().write() )
	uiMSG().warning( tr("Could not save Survey Data Root "
			    "location in the settings file") );
}


extern "C" { mGlobal(Basic) void SetCurBaseDataDirOverrule(const char*); }
#define mRetExitWin { SetCurBaseDataDirOverrule( "" ); return true; }


bool uiSurvey::acceptOK( CallBacker* )
{
    if ( !dirfld_ ) mRetExitWin
    if ( dirfld_->isEmpty() )
	mErrRet("Please create a survey (or press Cancel)")

    const BufferString selsurv( selectedSurveyName() );
    const bool samedataroot = dataroot_ == orgdataroot_;
    const bool samesurvey = samedataroot && initialsurveyname_ == selsurv;

    // Step 1: write local changes
    if ( !writeSurvInfoFileIfCommentChanged() )
	mErrRet(0)
    if ( samedataroot && samesurvey && !parschanged_ )
	mRetExitWin

    // Step 2: write default/current survey file
    if ( !writeSettingsSurveyFile() )
	mErrRet(0)

    // Step 3: record data root preference
    if ( !samedataroot )
	updateDataRootInSettings();

    // Step 4: Do the IOMan changes necessary
    if ( samesurvey )
	IOM().surveyParsChanged();
    else
    {
	if ( !samedataroot )
	{
	    if ( !uiSetDataDir::setRootDataDir(this,dataroot_) )
		return false;
	}
	if ( IOMan::newSurvey(cursurvinfo_) )
	{
	    cursurvinfo_ = 0; // it's not ours anymore
	    if ( survmap_ )
		survmap_->setSurveyInfo( 0 );
	}
	else
	{
	    const SurveyInfo* si = &SI();
	    if ( cursurvinfo_ == si )
	    {
		cursurvinfo_ = 0;
		if ( survmap_ )
		    survmap_->setSurveyInfo( 0 );
	    }

	    const bool isblocked = IOM().message().isEmpty();
	    if ( !isblocked )
		uiMSG().error( IOM().message() );
	    return false;
	}
    }

    // Step 5: start importing if possible
    if ( impiop_ && impsip_ )
    {
	readSurvInfoFromFile();
	const char* askq = impsip_->importAskQuestion();
	if ( askq && *askq && uiMSG().askGoOn(askq) )
	{
	    IOM().to( "100010" );
	    impsip_->startImport( parent(), *impiop_ );
	}
    }

    mRetExitWin
}


bool uiSurvey::rejectOK( CallBacker* )
{
    if ( cursurvremoved_ )
	mErrRet( "You have removed the current survey ...\n"
		   "You *have to* select a survey now!" )

    mRetExitWin
}


void uiSurvey::setCurrentSurvInfo( SurveyInfo* newsi, bool updscreen )
{
    delete cursurvinfo_; cursurvinfo_ = newsi;
    if ( updscreen )
	putToScreen();
    else if ( survmap_ )
	survmap_->setSurveyInfo( 0 );
}


void uiSurvey::rollbackNewSurvey( const char* errmsg )
{
    FilePath fp( cursurvinfo_->datadir_, cursurvinfo_->dirname_ );
    const bool haverem = File::removeDir( fp.fullPath() );
    setCurrentSurvInfo( 0, false );
    readSurvInfoFromFile();
    if ( errmsg && *errmsg )
    {
	const BufferString tousr( haverem ? "New survey removed because:\n"
		: "New survey directory is invalid because:\n", errmsg );
	uiMSG().error( tousr );
    }
}


#define mRetRollBackNewSurvey(errmsg) { rollbackNewSurvey(errmsg); return; }


void uiSurvey::newButPushed( CallBacker* )
{
    if ( !rootDirWritable() ) return;

    FilePath fp( GetSoftwareDir(0), "data", SurveyInfo::sKeyBasicSurveyName());
    SurveyInfo* newsurvinfo = SurveyInfo::read( fp.fullPath() );
    if ( !newsurvinfo )
    {
	BufferString errmsg( "Cannot read software default survey\n" );
	errmsg.add( "Try to reinstall the OpendTect package" );
	mErrRetVoid( errmsg )
    }

    uiStartNewSurveySetup dlg( this, dataroot_, *newsurvinfo );
    if ( !dlg.go() )
	{ delete newsurvinfo; return; }

    const BufferString orgdirname = newsurvinfo->getDirName().buf();
    const BufferString storagedir = FilePath( dataroot_ ).add( orgdirname )
							    .fullPath();
    if ( !uiSurveyInfoEditor::copySurv(
		mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),0,
				  dataroot_,orgdirname) )
	{ delete newsurvinfo;
	    mErrRetVoid( "Cannot make a copy of the default survey" ); }

    setCurrentSurvInfo( newsurvinfo, false );

    cursurvinfo_->datadir_ = dataroot_;
    if ( !File::makeWritable(storagedir,true,true) )
	mRetRollBackNewSurvey("Cannot set the permissions for the new survey")

    if ( !cursurvinfo_->write(dataroot_) )
	mRetRollBackNewSurvey( "Failed to write survey info" )

    if ( !doSurvInfoDialog(true) )
	mRetRollBackNewSurvey( 0 )
    else
    {
	readSurvInfoFromFile(); // essential
	putToScreen();
    }

    rmbut_->setSensitive(true);
    editbut_->setSensitive(true);
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive(true);
}


void uiSurvey::rmButPushed( CallBacker* )
{
    const BufferString selnm( selectedSurveyName() );
    const BufferString seldirnm = FilePath(dataroot_).add(selnm).fullPath();
    const BufferString truedirnm = getTrueDir( seldirnm );

    BufferString msg( "This will remove the entire survey directory:\n\t" );
    msg += selnm;
    msg += "\nFull path: "; msg += truedirnm;
    if ( !uiMSG().askRemove( msg ) ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    const bool rmisok = File::remove( truedirnm );
    MouseCursorManager::restoreOverride();
    if ( !rmisok )
	uiMSG().error( BufferString( truedirnm, "\nnot removed properly" ));

    if ( seldirnm != truedirnm ) // must have been a link
	if ( !File::remove(seldirnm) )
	    uiMSG().error( tr("Could not remove link to the removed survey") );

    updateSurvList();
    const char* ptr = GetSurveyName();
    if ( ptr && selnm == ptr )
    {
	cursurvremoved_ = true;
	if ( button(CANCEL) ) button(CANCEL)->setSensitive( false );
    }
}


void uiSurvey::editButPushed( CallBacker* )
{
    if ( !cursurvinfo_ ) return; // defensive
    if ( doSurvInfoDialog(false) )
	putToScreen();
}


void uiSurvey::copyButPushed( CallBacker* )
{
    if ( !cursurvinfo_ || !rootDirWritable() ) return;

    uiNewSurveyByCopy dlg( this, dataroot_, selectedSurveyName() );
    if ( !dlg.go() )
	return;

    setCurrentSurvInfo( SurveyInfo::read(dlg.newdirnm_) );
    if ( !cursurvinfo_ )
	mErrRetVoid( "Could not read the copied survey" )

    cursurvinfo_->setName( FilePath(dlg.newdirnm_).fileName() );
    cursurvinfo_->updateDirName();
    if ( !cursurvinfo_->write() )
	uiMSG().warning( tr("Could not write updated survey info") );

    updateSurvList();
    dirfld_->setCurrentItem( dlg.newdirnm_ );
}


void uiSurvey::importButPushed( CallBacker* )
{
    if ( !rootDirWritable() ) return;

    uiFileDialog fdlg( this, true, 0, "*.zip", tr("Select survey zip file") );
    fdlg.setSelectedFilter( sZipFileMask );
    if ( !fdlg.go() )
	return;

    uiSurvey_UnzipFile( this, fdlg.fileName(), dataroot_ );
    updateSurvList();
    //TODO set unpacked survey as current with dirfld_->setCurrentItem()
}


void uiSurvey::exportButPushed( CallBacker* )
{
    const BufferString survnm( selectedSurveyName() );
    const BufferString title( "Compress ", survnm, " survey as zip archive" );
    uiDialog dlg( this,
    uiDialog::Setup(title,mNoDlgTitle,
                    mODHelpKey(mSurveyexportButPushedHelpID) ));
    uiFileInput* fnmfld = new uiFileInput( &dlg,"Select output destination",
		    uiFileInput::Setup().directories(false).forread(false)
		    .allowallextensions(false));
    fnmfld->setDefaultExtension( "zip" );
    fnmfld->setFilter( sZipFileMask );
    uiLabel* sharfld = new uiLabel( &dlg,
			  tr("You can share surveys to Open Seismic Repository."
			   "To know more ") );
    sharfld->attach( leftAlignedBelow,  fnmfld );
    uiPushButton* osrbutton = new uiPushButton( &dlg,
				   tr("Click here"), mSCB(osrbuttonCB), false );
    osrbutton->attach( rightOf, sharfld );
    if ( !dlg.go() )
	return;

    FilePath exportzippath( fnmfld->fileName() );
    BufferString zipext = exportzippath.extension();
    if ( zipext != "zip" )
	mErrRetVoid( "Please add .zip extension to the file name" )

    uiSurvey_ZipDirectory( this, survnm, exportzippath.fullPath() );
}


void uiSurvey::dataRootPushed( CallBacker* )
{
    uiSetDataDir dlg( this );
    if ( !dlg.go() || dataroot_ == dlg.selectedDir() )
	return;

    dataroot_ = dlg.selectedDir();
    SetCurBaseDataDirOverrule( dataroot_ );

    updateSurvList();
    updateDataRootLabel();
    const char* ptr = GetSurveyName();
    if ( ptr && dirfld_->isPresent(ptr) )
	dirfld_->setCurrentItem( GetSurveyName() );

    selChange( 0 );
}


void uiSurvey::utilButPush( CallBacker* cb )
{
    if ( !cursurvinfo_ )
	return;
    mDynamicCastGet(uiButton*,tb,cb)
    if ( !tb )
	{ pErrMsg("Huh"); return; }

    const int butidx = utilbuts_.indexOf( tb );
    if ( butidx < 0 ) { pErrMsg("Huh"); return; }

    if ( butidx == 0 )
    {
	uiConvertPos dlg( this, *cursurvinfo_ );
	dlg.go();
    }
    else if ( butidx == 1 )
    {
	if ( !cursurvinfo_ ) return;
	uiLatLong2CoordDlg dlg( this, cursurvinfo_->latlong2Coord(),
					cursurvinfo_ );
	if ( dlg.go() && !cursurvinfo_->write() )
	    mErrRetVoid( "Could not write the setup" )
    }
    else
    {
	Util* util = getUtils()[butidx];
	util->cb_.doCall( this );
    }
}


void uiSurvey::updateDataRootLabel()
{
    datarootlbl_->setText( dataroot_.buf() );
}


void uiSurvey::updateSurvList()
{
    NotifyStopper ns( dirfld_->selectionChanged );
    const BufferString prevsel( dirfld_->getText() );
    dirfld_->setEmpty();
    BufferStringSet dirlist; getSurveyList( dirlist, dataroot_ );
    dirfld_->addItems( dirlist );

    if ( !dirfld_->isEmpty() )
    {
	int selidx = -1;
	if ( cursurvinfo_ )
	    selidx = dirfld_->indexOf( cursurvinfo_->getDirName() );
	if ( selidx < 0 )
	    selidx = dirfld_->indexOf( prevsel );
	dirfld_->setCurrentItem( selidx < 0 ? 0 : selidx );
    }
}


bool uiSurvey::writeSettingsSurveyFile()
{
    if ( dirfld_->isEmpty() )
	{ pErrMsg( "No survey in the list" ); return false; }

    BufferString seltxt( selectedSurveyName() );
    if ( seltxt.isEmpty() )
	mErrRet( "Survey folder name cannot be empty" )

    if ( !File::exists(FilePath(dataroot_,seltxt).fullPath()) )
	mErrRet( "Survey directory does not exist anymore" )

    const char* survfnm = SurveyInfo::surveyFileName();
    if ( !survfnm )
	mErrRet( "Internal error: cannot construct last-survey-filename" )

    od_ostream strm( survfnm );
    if ( !strm.isOK() )
	mErrRet( BufferString("Cannot open ",survfnm," for write") )

    strm << seltxt;
    if ( !strm.isOK() )
	mErrRet( BufferString("Error writing to ",survfnm) )

    return true;
}


void uiSurvey::readSurvInfoFromFile()
{
    const BufferString survnm( selectedSurveyName() );
    SurveyInfo* newsi = 0;
    if ( !survnm.isEmpty() )
    {
	const BufferString fname = FilePath( dataroot_ )
			    .add( selectedSurveyName() ).fullPath();
	newsi = SurveyInfo::read( fname );
	if ( !newsi )
	    uiMSG().warning(
		    BufferString("Cannot read survey setup file: ",fname) );
    }

    if ( newsi )
	setCurrentSurvInfo( newsi );
}


bool uiSurvey::doSurvInfoDialog( bool isnew )
{
    delete impiop_; impiop_ = 0; impsip_ = 0;
    uiSurveyInfoEditor dlg( this, *cursurvinfo_, isnew );
    if ( !dlg.isOK() )
	return false;

    dlg.survParChanged.notify( mCB(this,uiSurvey,updateInfo) );
    if ( !dlg.go() )
	return false;

    if ( initialsurveyname_ == selectedSurveyName() )
	parschanged_ = true;

    updateSurvList();
    dirfld_->setCurrentItem( dlg.dirName() );

    impiop_ = dlg.impiop_; dlg.impiop_ = 0;
    impsip_ = dlg.lastsip_;
    return true;
}


void uiSurvey::selChange( CallBacker* )
{
    if ( dirfld_->isEmpty() )
	return;

    writeSurvInfoFileIfCommentChanged();
    readSurvInfoFromFile();
    putToScreen();
}


void uiSurvey::putToScreen()
{
    if ( !survmap_ ) return;

    survmap_->setSurveyInfo( cursurvinfo_ );

    BufferString locinfo( "Location: " );
    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString zinfo( "Z range" );
    BufferString bininfo( "Inl/Crl bin size" );
    BufferString areainfo( "Area" );
    BufferString survtypeinfo( "Survey type: " );
    BufferString orientinfo( "In-line Orientation: " );

    if ( !cursurvinfo_ )
    {
	notesfld_->setText( "" );
	zinfo.add( ":" ); bininfo.add( ":" ); areainfo.add( ":" );
    }
    else
    {
	const SurveyInfo& si = *cursurvinfo_;
	notesfld_->setText( si.comment() );

	zinfo.add( " (" ).add( si.getZUnitString(false) ).add( "): ");
	bininfo.add( " (" ).add( si.getXYUnitString(false) ).add( "/line): " );
	areainfo.add( " (sq " ).add( si.xyInFeet() ? "mi" : "km" ).add( "): ");

	if ( si.sampling(false).hrg.totalNr() > 0 )
	{
	    inlinfo.add( si.sampling(false).hrg.start.inl() );
	    inlinfo.add( " - ").add( si.sampling(false).hrg.stop.inl() );
	    inlinfo.add( " - " ).add( si.inlStep() );
	    crlinfo.add( si.sampling(false).hrg.start.crl() );
	    crlinfo.add( " - ").add( si.sampling(false).hrg.stop.crl() );
	    crlinfo.add( " - " ).add( si.crlStep() );

	    const float inldist = si.inlDistance(), crldist = si.crlDistance();

	    bininfo.add( toString(inldist,2) ).add( "/" )
		   .add( toString(crldist,2) );
	    float area = (float) ( si.getArea(false) * 1e-6 ); //in km2
	    if ( si.xyInFeet() )
		area /= 2.590; // square miles

	    areainfo.add( toString(area,2) );
	}

	#define mAdd2ZString(nr) zinfo += istime ? mNINT32(1000*nr) : nr;

	const bool istime = si.zIsTime();
	mAdd2ZString( si.zRange(false).start );
	zinfo += " - "; mAdd2ZString( si.zRange(false).stop );
	zinfo += " - "; mAdd2ZString( si.zRange(false).step );
	survtypeinfo.add( SurveyInfo::toString(si.survDataType()) );

	FilePath fp( si.datadir_, si.dirname_ );
	fp.makeCanonical();
	locinfo.add( fp.fullPath() );

	const float usrang = Math::degFromNorth( si.angleXInl() );
	orientinfo.add( toString(usrang,2) ).add( " Degrees from N" );
    }

    BufferString infostr;
    infostr.add( inlinfo ).addNewLine().add( crlinfo ).addNewLine()
	.add( zinfo ).addNewLine().add( bininfo ).addNewLine()
	.add( areainfo ).addNewLine().add( survtypeinfo )
	.addNewLine().add( locinfo ).addNewLine().add( orientinfo );
    infofld_->setText( infostr );

    const bool anysvy = !dirfld_->isEmpty();
    rmbut_->setSensitive( anysvy );
    editbut_->setSensitive( anysvy );
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive( anysvy );
}


bool uiSurvey::writeSurvInfoFileIfCommentChanged()
{
    if ( !cursurvinfo_ || !notesfld_->isModified() )
	return true;

    cursurvinfo_->setComment( notesfld_->text() );
    if ( !cursurvinfo_->write( dataroot_ ) )
	mErrRet( "Failed to write survey info.\nNo changes committed." )

    return true;
}
