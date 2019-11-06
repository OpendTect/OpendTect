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
#include "uichecklist.h"
#include "uicombobox.h"
#include "uiconvpos.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uifont.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilatlong2coord.h"
#include "uilineedit.h"
#include "uilistbox.h"
#include "uimain.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uiseparator.h"
#include "uisetdatadir.h"
#include "uisettings.h"
#include "uisip.h"
#include "uisplitter.h"
#include "uisurveyselect.h"
#include "uisurveyzip.h"
#include "uisurvinfoed.h"
#include "uisurvmap.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "ui2dsip.h"

#include "angles.h"
#include "ctxtioobj.h"
#include "trckeyzsampling.h"
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
#include "nrbytes2string.h"
#include "oddirs.h"
#include "odver.h"
#include "od_ostream.h"
#include "od_helpids.h"
#include "settings.h"
#include "survinfo.h"
#include "systeminfo.h"


static const char*	sZipFileMask = "ZIP files (*.zip *.ZIP)";
#define mErrRetVoid(s)	{ if ( s.isSet() ) uiMSG().error(s); return; }
#define mErrRet(s)	{ if ( s.isSet() ) uiMSG().error(s); return false; }

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
	*newutils += new uiSurvey::Util( "xy2ic",od_static_tr("getUtils",
		"Convert (X,Y) to/from (%1,%2)").arg(uiStrings::sInline())
		.arg(uiStrings::sCrossline()), CallBack() );

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
{ mODTextTranslationClass(uiNewSurveyByCopy);

public:

uiNewSurveyByCopy( uiParent* p, const char* dataroot, const char* dirnm )
	: uiDialog(p,uiDialog::Setup(uiStrings::phrCopy(uiStrings::sSurvey()),
				     mNoDlgTitle,
				     mODHelpKey(mNewSurveyByCopyHelpID)))
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
	uiMSG().error(tr("A survey '%1' already exists.\n"
			 "You will have to remove it first").arg(newdirnm_));
	return false;
    }

    uiTaskRunner taskrunner( this );
    const BufferString fromdir = getTrueDir( inpdirnm_ );
    PtrMan<Executor> copier = File::getRecursiveCopier( fromdir, newdirnm_ );
    if ( !taskrunner.execute(*copier) )
	{ uiMSG().error(tr("Cannot copy the survey data")); return false; }

    File::makeWritable( newdirnm_, true, true );
    return true;
}

bool acceptOK( CallBacker* )
{
    if ( !inpsurveyfld_->getFullSurveyPath( inpdirnm_ ) ||
	 !newsurveyfld_->getFullSurveyPath( newdirnm_) )
	mErrRet(tr("No Valid or Empty Input"))

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
{ mODTextTranslationClass(uiStartNewSurveySetup);

public:
			uiStartNewSurveySetup(uiParent*,const char*,
					      SurveyInfo&);

    bool		isOK();
    bool		acceptOK(CallBacker*);

    ObjectSet<uiSurvInfoProvider> sips_;
    int			sipidx_;

protected:

    const BufferString	dataroot_;
    SurveyInfo&		survinfo_;
    uiGenInput*		survnmfld_;
    uiGenInput*		zistimefld_;
    uiGenInput*		zinfeetfld_;
    uiCheckList*	pol2dfld_;
    uiListBox*		sipfld_;

    BufferString	sipName() const;
    BufferString	survName() const { return survnmfld_->text(); }
    bool		has3D() const	 { return pol2dfld_->isChecked(0); }
    bool		has2D() const	 { return pol2dfld_->isChecked(1); }
    bool		isTime() const	 { return zistimefld_->getBoolValue();}
    bool		isInFeet() const { return !zinfeetfld_->getBoolValue();}

    void		fillSipsFld(bool have2d,bool have3d);

SurveyInfo::Pol2D pol2D() const
{
    return has3D() ? ( has2D() ? SurveyInfo::Both2DAnd3D
			       : SurveyInfo::No2D )
			       : SurveyInfo::Only2D;
}

void pol2dChg( CallBacker* cb )
{
    fillSipsFld( has2D(), has3D() );
}

void zdomainChg( CallBacker* cb )
{
    zinfeetfld_->display( !isTime() );
}

};


uiStartNewSurveySetup::uiStartNewSurveySetup(uiParent* p, const char* dataroot,
					      SurveyInfo& survinfo )
	: uiDialog(p,Setup(tr("Create New Survey"),
			   tr("Specify new survey parameters"),
			   mODHelpKey(mStartNewSurveySetupHelpID)))
	, survinfo_(survinfo)
	, dataroot_(dataroot)
	, sips_(uiSurveyInfoEditor::survInfoProvs())
	, sipidx_(-1)
{
    setOkText( uiStrings::sNext() );

    survnmfld_ = new uiGenInput( this, tr("Survey name") );
    survnmfld_->setElemSzPol( uiObject::Wide );

    pol2dfld_ = new uiCheckList( this, uiCheckList::OneMinimum, OD::Horizontal);
    pol2dfld_->setLabel( tr("Available data") );
    pol2dfld_->addItem( uiStrings::s3D() ).addItem( uiStrings::s2D() );
    pol2dfld_->setChecked( 0, true ).setChecked( 1, true );
    pol2dfld_->changed.notify( mCB(this,uiStartNewSurveySetup,pol2dChg) );
    pol2dfld_->attach( alignedBelow, survnmfld_ );

    for ( int idx=0; idx<sips_.size(); idx++ )
    {
	if ( !sips_[idx]->isAvailable() )
	    { sips_.removeSingle( idx ); idx--; }
    }

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Initial setup") );
    sipfld_ = new uiListBox( this, su );
    sipfld_->attach( alignedBelow, pol2dfld_ );
    sipfld_->setPrefHeightInChar( sips_.size() + 1 );

    zistimefld_ = new uiGenInput( this, tr("Z Domain"),
		BoolInpSpec(true,uiStrings::sTime(),uiStrings::sDepth()) );
    zistimefld_->valuechanged.notify(
			mCB(this,uiStartNewSurveySetup,zdomainChg) );
    zistimefld_->attach( alignedBelow, sipfld_ );

    zinfeetfld_ = new uiGenInput( this, tr("Depth unit"),
				BoolInpSpec(true,tr("Meter"),tr("Feet")) );
    zinfeetfld_->attach( alignedBelow, zistimefld_ );
    zinfeetfld_->display( !isTime() );

    fillSipsFld( has2D(), has3D() );
}


bool uiStartNewSurveySetup::isOK()
{
    BufferString survnm = survName();
    if ( survnm.isEmpty() )
	mErrRet(tr("Please enter a new survey name"))

    survnm.clean( BufferString::AllowDots );
    const BufferString storagedir = FilePath(dataroot_).add(survnm).fullPath();
    if ( File::exists(storagedir) )
    {
	uiString errmsg = tr("A survey called %1 already exists\nPlease "
			     "remove it first or use another survey name")
			.arg(survnm);
	mErrRet( errmsg )
    }

    sipidx_ = sipfld_->currentItem();
    if ( !sips_.validIdx(sipidx_) )
	sipidx_ = -1;

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
    survinfo_.setSipName( sipName() );

    return true;
}


BufferString uiStartNewSurveySetup::sipName() const
{
    const int sipidx = sipfld_->currentItem();
    return sips_.validIdx(sipidx) ? sips_[sipidx]->usrText() : "";
}


void uiStartNewSurveySetup::fillSipsFld( bool have2d, bool have3d )
{
    int preferredsel = sipfld_->isEmpty() ? -1 : sipfld_->currentItem();
    sipfld_->setEmpty();

    const int nrprovs = sips_.size();
    for ( int idx=0; idx<nrprovs; idx++ )
    {
	uiSurvInfoProvider& sip = *sips_[idx];
	mDynamicCastGet(const ui2DSurvInfoProvider*,sip2d,&sip);

	if ( preferredsel < 0 )
	{
	    if ( FixedString(sip.usrText()).contains("etrel") )
		preferredsel = idx;
	    else
	    {
		if ( sip2d && !have3d )
		    preferredsel = idx;
	    }
	}

	sipfld_->addItem( toUiString(sip.usrText()) );
	const char* icnm = sip.iconName();
	if ( !icnm || !*icnm )
	    icnm = "empty";
	sipfld_->setIcon( idx, icnm );
	if ( !have2d && sip2d )
	    sipfld_->setItemSelectable( sipfld_->size()-1, false );
    }

    sipfld_->addItem( tr("Enter by hand") ); // always last
    sipfld_->setIcon( sipfld_->size()-1, "manualenter" );
    sipfld_->setCurrentItem( preferredsel < 0 ? 0 : preferredsel );

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
    , freshsurveyselected_(false)
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
    mDefineStaticLocalObject( int, sipidxnav, mUnusedVar =
	    uiSurveyInfoEditor::addInfoProvider(new uiNavSurvInfoProvider) );
    mDefineStaticLocalObject( int, sipidxcp, mUnusedVar =
	    uiSurveyInfoEditor::addInfoProvider(new uiCopySurveySIP) );
    mDefineStaticLocalObject( int, sipidxfile, mUnusedVar =
	    uiSurveyInfoEditor::addInfoProvider(new uiSurveyFileSIP) );

    uiGroup* topgrp = new uiGroup( this, "TopGroup" );
    uiPushButton* datarootbut =
		new uiPushButton( topgrp, tr("Survey Data Root"), false );

    datarootbut->setIcon( "database" );
    datarootbut->activated.notify( mCB(this,uiSurvey,dataRootPushed) );
    datarootbut->attach( leftBorder );

    datarootlbl_ = new uiLineEdit( topgrp, "Data Root Label" );
    datarootlbl_->setHSzPol( uiObject::WideMax );
    datarootlbl_->setReadOnly();
    datarootlbl_->setBackgroundColor( backgroundColor() );
    datarootlbl_->attach( rightOf, datarootbut );

    uiToolButton* infobut = new uiToolButton( topgrp, "info",
	tr("Data Root Information"), mCB(this,uiSurvey,dataRootInfoCB) );
    infobut->attach( rightTo, datarootlbl_ );

    uiToolButton* settbut = new uiToolButton( topgrp, "settings",
	tr("General Settings"), mCB(this,uiSurvey,odSettsButPush) );
    settbut->attach( rightOf, infobut );

    uiSeparator* sep1 = new uiSeparator( topgrp, "Separator 1" );
    sep1->attach( stretchedBelow, datarootbut );

    uiGroup* leftgrp = new uiGroup( topgrp, "Survey selection left" );
    uiGroup* rightgrp = new uiGroup( topgrp, "Survey selection right" );

    fillLeftGroup( leftgrp );
    fillRightGroup( rightgrp );
    leftgrp->attach( ensureBelow, sep1 );
    rightgrp->attach( rightOf, leftgrp );

    uiLabel* infolbl = new uiLabel( topgrp, uiString::emptyString() );
    infolbl->setPixmap( "info" );
    infolbl->setToolTip( tr("Survey Information") );
    infolbl->attach( alignedBelow, leftgrp );
    infofld_ = new uiTextEdit( topgrp, "Info", true );
    infofld_->setPrefHeightInChar( 8 );
    infofld_->setStretch( 2, 1 );
    infofld_->attach( rightTo, infolbl );
    infofld_->attach( ensureBelow, rightgrp );

    uiGroup* botgrp = new uiGroup( this, "Bottom Group" );
    uiLabel* notelbl = new uiLabel( botgrp, uiStrings::sEmptyString() );
    notelbl->setPixmap( uiPixmap("notes") );
    notelbl->setToolTip( tr("Notes") );
    notelbl->setMaximumWidth( 32 );

    notesfld_ = new uiTextEdit( botgrp, "Survey Notes" );
    notesfld_->attach( rightTo, notelbl );
    notesfld_->setPrefHeightInChar( 5 );
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
    ObjectSet<uiSurvInfoProvider>& survprovs =
					uiSurveyInfoEditor::survInfoProvs();
    for ( int idx=0; idx<survprovs.size(); idx++ )
    {
	mDynamicCastGet(uiCopySurveySIP*,uisurvcpsip,survprovs[idx])
	if ( !uisurvcpsip )
	    continue;

	uisurvcpsip->reset();
    }

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
    new uiToolButton( butgrp, "addnew", uiStrings::phrCreate(mJoinUiStrs(sNew(),
				sSurvey())), mCB(this,uiSurvey,newButPushed) );
    editbut_ = new uiToolButton( butgrp, "edit", tr("Edit Survey Parameters"),
				 mCB(this,uiSurvey,editButPushed) );
    new uiToolButton( butgrp, "copyobj",
	tr("Copy Survey"), mCB(this,uiSurvey,copyButPushed) );
    new uiToolButton( butgrp, "compress",
	tr("Compress survey as zip archive"),
	mCB(this,uiSurvey,exportButPushed) );
    new uiToolButton( butgrp, "extract",
	tr("Extract survey from zip archive"),
	mCB(this,uiSurvey,importButPushed) );
    new uiToolButton( butgrp, "share",
	tr("Share surveys through the OpendTect Seismic Repository"),
	mSCB(osrbuttonCB) );
    rmbut_ = new uiToolButton( butgrp, "delete", tr("Delete Survey"),
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
	uiString msg = tr("Cannot create new survey in\n"
			  "%1.\nDirectory is write protected.")
	    .arg(dataroot_);
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


void uiSurvey::updateSurveyNames()
{
    surveynames_.erase(); surveydirs_.erase();

    BufferString basedir = dataroot_;
    if ( basedir.isEmpty() )
	basedir = GetBaseDataDir();
    DirList dl( basedir, DirList::DirsOnly );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString& dirnm = dl.get( idx );
	const FilePath fp( basedir, dirnm, SurveyInfo::sKeySetupFileName() );
	if ( !File::exists(fp.fullPath()) )
	    continue;

	IOPar survpar; survpar.read( fp.fullPath(), "Survey Info", true );
	BufferString survname; survpar.get( sKey::Name(), survname );
	if ( !survname.isEmpty() )
	{
	    surveynames_.add( survname );
	    surveydirs_.add( dirnm );
	}
    }
}


bool uiSurvey::survTypeOKForUser( bool is2d )
{
    const bool dowarn = (is2d && !SI().has2D()) || (!is2d && !SI().has3D());
    if ( !dowarn ) return true;

 uiString warnmsg = tr("Your survey is set up as '%1 data\nyou will have to "
		       "change the survey setup.\n\nDo you wish to continue?")
		  .arg(is2d ? tr("3-D only'.\nTo be able to actually use 2-D")
			    : tr("2-D only'.\nTo be able to actually use 3-D"));

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
    if ( !dirfld_ )
	mRetExitWin
    if ( dirfld_->isEmpty() )
	mErrRet(tr("Please create a survey (or press Cancel)"))

    const BufferString selsurv( selectedSurveyName() );
    const bool samedataroot = dataroot_ == orgdataroot_;
    const bool samesurvey = samedataroot && initialsurveyname_ == selsurv;

    // Step 1: write local changes
    if ( !writeSurvInfoFileIfCommentChanged() )
	mErrRet(uiString::emptyString())
    if ( samedataroot && samesurvey && !parschanged_ )
	mRetExitWin

    // Step 2: write default/current survey file
    if ( !writeSettingsSurveyFile() )
	mErrRet(uiString::emptyString())

    // Step 3: record data root preference
    if ( !samedataroot )
	updateDataRootInSettings();

    // Step 4: Do the IOMan changes necessary
    if ( samesurvey )
    {
	if ( cursurvinfo_ )
	    eSI() = *cursurvinfo_;

	IOM().surveyParsChanged();
    }
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
		uiMSG().error( mToUiStringTodo(IOM().message()) );
	    return false;
	}
    }

    // Step 5: if fresh survey, help user on his/her way
    if ( impiop_ && impsip_ )
    {
	freshsurveyselected_ = true;
	readSurvInfoFromFile();
	const char* askq = impsip_->importAskQuestion();
	if ( askq && *askq && uiMSG().askGoOn(mToUiStringTodo(askq)) )
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
    {
	uiString msg( tr("You have removed the current survey.\n" ) );
	if ( !hasSurveys() )
	    msg.append( tr("No surveys found in the list."), true );

	msg.append( tr("Want to exit from OpendTect"),true );
	return uiMSG().askGoOn( msg );
    }

    mRetExitWin
}


bool uiSurvey::hasSurveys() const
{
    return dirfld_ && !dirfld_->isEmpty();
}


void uiSurvey::setCurrentSurvInfo( SurveyInfo* newsi, bool updscreen )
{
    delete cursurvinfo_; cursurvinfo_ = newsi;

    if ( updscreen )
	putToScreen();
    else if ( survmap_ )
	survmap_->setSurveyInfo( 0 );
}


void uiSurvey::rollbackNewSurvey( const uiString& errmsg )
{
    if ( !cursurvinfo_ )
	return;

    FilePath fp( cursurvinfo_->datadir_, cursurvinfo_->dirname_ );
    const bool haverem = File::removeDir( fp.fullPath() );
    setCurrentSurvInfo( 0, false );
    readSurvInfoFromFile();
    if ( !errmsg.isEmpty()  )
    {
	const uiString tousr = haverem ? tr("New survey removed because:\n%1")
		.arg(mToUiStringTodo(errmsg))
		: tr("New survey directory is invalid because:\n%1")
		.arg(mToUiStringTodo(errmsg) );
	uiMSG().error( tousr );
    }
}


#define mRetRollBackNewSurvey(errmsg) \
{ \
    rollbackNewSurvey(errmsg); \
    selChange(0); \
    return; \
}


void uiSurvey::newButPushed( CallBacker* )
{
    if ( !rootDirWritable() ) return;

    const FilePath fp( mGetSWDirDataDir(), SurveyInfo::sKeyBasicSurveyName());
    SurveyInfo* newsurvinfo = SurveyInfo::read( fp.fullPath() );
    if ( !newsurvinfo )
    {
	uiString errmsg = tr("Cannot read software default survey\n"
			     "Try to reinstall the OpendTect package");
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
	    mErrRetVoid( tr("Cannot make a copy of the default survey") ); }

    setCurrentSurvInfo( newsurvinfo, false );

    cursurvinfo_->datadir_ = dataroot_;
    if ( !File::makeWritable(storagedir,true,true) )
	mRetRollBackNewSurvey( tr("Cannot set the permissions "
				  "for the new survey") )

    if ( !cursurvinfo_->write(dataroot_) )
	mRetRollBackNewSurvey( tr("%1 Info").
			  arg(uiStrings::phrCannotWrite(uiStrings::sSurvey())) )

    if ( !doSurvInfoDialog(true) )
	mRetRollBackNewSurvey( uiStrings::sEmptyString() )
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

    uiString msg = tr("This will delete the entire survey directory:\n\t%1"
		      "\nFull path: %2").arg(selnm).arg(truedirnm);
    if ( !uiMSG().askRemove(msg) ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    const bool rmisok = File::remove( truedirnm );
    MouseCursorManager::restoreOverride();
    if ( !rmisok )
	uiMSG().error(tr("%1\nnot removed properly").arg(truedirnm));

    if ( seldirnm != truedirnm ) // must have been a link
	if ( !File::remove(seldirnm) )
	    uiMSG().error( tr("Could not remove link to the removed survey") );

    updateSurvList();
    const char* ptr = GetSurveyName();
    if ( ptr && selnm == ptr )
    {
	cursurvremoved_ = true;
	if ( button(CANCEL) )
	    button(CANCEL)->setText( tr("Exit") );
    }

    if ( dirfld_->isEmpty() )
	setCurrentSurvInfo( 0, true );

    selChange( 0 );
}


void uiSurvey::editButPushed( CallBacker* )
{
    if ( !cursurvinfo_ ) return; // defensive
    if ( doSurvInfoDialog(false) )
	putToScreen();
}


void uiSurvey::copyButPushed( CallBacker* )
{
    if ( !rootDirWritable() ) return;

    uiNewSurveyByCopy dlg( this, dataroot_, selectedSurveyName() );
    if ( !dlg.go() )
	return;

    setCurrentSurvInfo( SurveyInfo::read(dlg.newdirnm_) );
    if ( !cursurvinfo_ )
	mErrRetVoid(tr("Could not read the copied survey"))

    cursurvinfo_->setName( FilePath(dlg.newdirnm_).fileName() );
    cursurvinfo_->updateDirName();
    if ( !cursurvinfo_->write() )
	uiMSG().warning( tr("Could not write updated survey info") );

    updateSurvList();
    dirfld_->setCurrentItem( dlg.newdirnm_ );
    putToScreen();
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
    readSurvInfoFromFile();
    //TODO set unpacked survey as current with dirfld_->setCurrentItem()
}


void uiSurvey::exportButPushed( CallBacker* )
{
    const char* survnm( selectedSurveyName() );
    const uiString title = tr("Compress %1 survey as zip archive")
						.arg(survnm);
    uiDialog dlg( this,
    uiDialog::Setup(title,mNoDlgTitle,
		    mODHelpKey(mSurveyCompressButPushedHelpID) ));
    uiFileInput* fnmfld = new uiFileInput( &dlg,uiStrings::phrSelect(
		    uiStrings::phrOutput(tr("Destination"))),
		    uiFileInput::Setup().directories(false).forread(false)
		    .allowallextensions(false));
    fnmfld->setDefaultExtension( "zip" );
    fnmfld->setFilter( sZipFileMask );
    uiLabel* sharfld = new uiLabel( &dlg,
			  tr("You can share surveys to Open Seismic Repository."
			   "To know more ") );
    sharfld->attach( leftAlignedBelow,  fnmfld );
    uiPushButton* osrbutton =
	new uiPushButton( &dlg, tr("Click here"), mSCB(osrbuttonCB), false );
    osrbutton->attach( rightOf, sharfld );
    if ( !dlg.go() )
	return;

    FilePath exportzippath( fnmfld->fileName() );
    BufferString zipext = exportzippath.extension();
    if ( zipext != "zip" )
	mErrRetVoid(tr("Please add .zip extension to the file name"))

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
    if ( dirfld_->isEmpty() )
    {
	setCurrentSurvInfo( 0 , true );
	return;
    }

    const char* ptr = GetSurveyName();
    if ( ptr && dirfld_->isPresent(ptr) )
	dirfld_->setCurrentItem( GetSurveyName() );

    selChange( 0 );
}


static BufferString getSizeStr( od_int64 nrb )
{
    NrBytesToStringCreator conv( nrb );
    return conv.getString( nrb, 3 );
}


void uiSurvey::dataRootInfoCB( CallBacker* )
{
    const BufferString fsnm = System::fileSystemName( dataroot_ );
    const BufferString fstp = System::fileSystemType( dataroot_ );
    const BufferString totalmem = getSizeStr( System::bytesTotal(dataroot_) );
    const BufferString freemem = getSizeStr( System::bytesFree(dataroot_) );
    const BufferString availmem = getSizeStr(System::bytesAvailable(dataroot_));
    uiString msg = tr("%1: %2\n%3: %4\n%5: %6\n%7: %8\n%9: %10")
	.arg(sKey::Name()).arg(fsnm)
	.arg(sKey::Type()).arg(fstp)
	.arg(tr("Total memory")).arg(totalmem)
	.arg(tr("Free memory")).arg(freemem)
	.arg(tr("Available memory")).arg(availmem);
    uiMSG().message( msg );
}


void uiSurvey::odSettsButPush( CallBacker* )
{
    uiSettingsDlg dlg( this );
    dlg.go();
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
    else
    {
	Util* util = getUtils()[butidx];
	util->cb_.doCall( this );
    }
}


void uiSurvey::updateDataRootLabel()
{
    datarootlbl_->setText( dataroot_ );
}


void uiSurvey::updateSurvList()
{
    NotifyStopper ns( dirfld_->selectionChanged );
    int newselidx = dirfld_->currentItem();
    const BufferString prevsel( dirfld_->getText() );
    dirfld_->setEmpty();
    BufferStringSet dirlist; getSurveyList( dirlist, dataroot_ );
    dirfld_->addItems( dirlist );
//    updateSurveyNames();
//    dirfld_->addItems( surveynames_ );

    if ( dirfld_->isEmpty() )
	return;

    const int idxofprevsel = dirfld_->indexOf( prevsel );
    const int idxofcursi = cursurvinfo_ ? dirfld_->indexOf(
					cursurvinfo_->getDirName() ) : -1;
    if ( idxofcursi >= 0 )
	newselidx = idxofcursi;
    else if ( idxofprevsel >= 0 )
	newselidx = idxofprevsel;

    if ( newselidx < 0 )
	newselidx = 0;
    if ( newselidx >= dirfld_->size() )
	newselidx = dirfld_->size()-1 ;

    dirfld_->setCurrentItem( newselidx );
}


bool uiSurvey::writeSettingsSurveyFile()
{
    if ( dirfld_->isEmpty() )
	{ pErrMsg( "No survey in the list" ); return false; }

    BufferString seltxt( selectedSurveyName() );
    if ( seltxt.isEmpty() )
	mErrRet(tr("Survey folder name cannot be empty"))

    if ( !File::exists(FilePath(dataroot_,seltxt).fullPath()) )
	mErrRet(tr("Survey directory does not exist anymore"))

    const char* survfnm = SurveyInfo::surveyFileName();
    if ( !survfnm )
	mErrRet(tr("Internal error: cannot construct last-survey-filename"))

    od_ostream strm( survfnm );
    if ( !strm.isOK() )
	mErrRet(tr("Cannot open %1 for write").arg(survfnm))

    strm << seltxt;
    if ( !strm.isOK() )
	mErrRet( tr("Error writing to %1").arg(survfnm) )

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
		    tr("Cannot read survey setup file: %1").arg(fname) );
    }

    if ( newsi )
	setCurrentSurvInfo( newsi );
}


// Needed because uiSurveyInfoEditor will destruct cursurvinfo_ if isnew
#define mRetSafe(rv) { \
    if ( isnew ) cursurvinfo_ = 0; \
    return rv; }


bool uiSurvey::doSurvInfoDialog( bool isnew )
{
    delete impiop_; impiop_ = 0; impsip_ = 0;
    uiSurveyInfoEditor dlg( this, *cursurvinfo_, isnew );
    if ( !dlg.isOK() )
	mRetSafe( false )

    dlg.survParChanged.notify( mCB(this,uiSurvey,updateInfo) );
    if ( !dlg.go() )
    {
	if ( !isnew )
	    readSurvInfoFromFile();

	mRetSafe( false )
    }

    if ( initialsurveyname_ == selectedSurveyName() )
	parschanged_ = true;

    updateSurvList();
    dirfld_->setCurrentItem( dlg.dirName() );

    impiop_ = dlg.impiop_; dlg.impiop_ = 0;
    impsip_ = dlg.lastsip_;

    mRetSafe( true )
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
    const bool hassurveys = !dirfld_->isEmpty();
    rmbut_->setSensitive( hassurveys );
    editbut_->setSensitive( hassurveys );
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive( hassurveys );

    if ( !hassurveys )
    {
	notesfld_->setText( uiString::emptyString() );
	infofld_->setText( uiString::emptyString() );
	return;
    }

    BufferString locinfo( "Location: " );
    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString zinfo( "Z range" );
    BufferString bininfo( "Inl/Crl bin size" );
    BufferString crsinfo( "CRS: " );
    BufferString areainfo( "Area: " );
    BufferString survtypeinfo( "Survey type: " );
    BufferString orientinfo( "In-line Orientation: " );

    if ( !cursurvinfo_ )
    {
	notesfld_->setText( "" );
	zinfo.add( ":" ); bininfo.add( ":" );
    }
    else
    {
	const SurveyInfo& si = *cursurvinfo_;
	areainfo.add( getAreaString(si.getArea(false),si.xyInFeet(),2,true) );
	notesfld_->setText( si.comment() );

	zinfo.add( "(" )
	     .add( si.zIsTime() ? ZDomain::Time().unitStr()
				: getDistUnitString(si.zInFeet(), false) )
	     .add( "): " );

	if ( si.getCoordSystem() )
	    crsinfo.add( si.getCoordSystem()->summary() );

	bininfo.add( " (" ).add( si.getXYUnitString(false) ).add( "/line): " );

	if ( si.sampling(false).hsamp_.totalNr() > 0 )
	{
	    inlinfo.add( si.sampling(false).hsamp_.start_.inl() );
	    inlinfo.add( " - ").add( si.sampling(false).hsamp_.stop_.inl() );
	    inlinfo.add( " - " ).add( si.inlStep() );
	    crlinfo.add( si.sampling(false).hsamp_.start_.crl() );
	    crlinfo.add( " - ").add( si.sampling(false).hsamp_.stop_.crl() );
	    crlinfo.add( " - " ).add( si.crlStep() );

	    const float inldist = si.inlDistance(), crldist = si.crlDistance();

	    bininfo.add( toString(inldist,2) ).add( "/" );
	    bininfo.add( toString(crldist,2) );
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
	.add( crsinfo ).addNewLine()
	.add( areainfo ).add( "; ").add( survtypeinfo ).addNewLine()
	.add( orientinfo ).addNewLine().add( locinfo );
    infofld_->setText( infostr );

}


bool uiSurvey::writeSurvInfoFileIfCommentChanged()
{
    if ( !cursurvinfo_ || !notesfld_->isModified() )
	return true;

    cursurvinfo_->setComment( notesfld_->text() );
    if ( !cursurvinfo_->write( dataroot_ ) )
	mErrRet(tr("Failed to write survey info.\nNo changes committed."))

    return true;
}
