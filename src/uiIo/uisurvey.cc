/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisurvey.h"

#include "uibuttongroup.h"
#include "uichecklist.h"
#include "uiclipboard.h"
#include "uiconvpos.h"
#include "uidatarootsel.h"
#include "uidesktopservices.h"
#include "uifileinput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisetdatadir.h"
#include "uisettings.h"
#include "uisip.h"
#include "uisplitter.h"
#include "uisurveyselect.h"
#include "uisurveyzip.h"
#include "uisurvinfoed.h"
#include "uisurvmap.h"
#include "uitabstack.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "ui2dsip.h"

#include "applicationdata.h"
#include "ctxtioobj.h"
#include "dirlist.h"
#include "executor.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "moddepmgr.h"
#include "mousecursor.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "settings.h"
#include "surveydisklocation.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#include <iostream>


static const char*	sZipFileMask = "ZIP files (*.zip *.ZIP)";
#define mErrRetVoid(s)	{ if ( s.isSet() ) uiMSG().error(s); return; }
#define mErrRet(s)	{ if ( s.isSet() ) uiMSG().error(s); return false; }

static int sMapWidth = 300;
static int sMapHeight = 300;

//--- General tools


static ObjectSet<uiSurvey::Util>& getUtils()
{
    mDefineStaticLocalObject( PtrMan<ManagedObjectSet<uiSurvey::Util> >,
			      utils, = nullptr )
    if ( !utils )
    {
	ManagedObjectSet<uiSurvey::Util>* newutils =
				    new ManagedObjectSet<uiSurvey::Util>;
	*newutils += new uiSurvey::Util( "xy2ic",od_static_tr("getUtils",
		"Convert (X,Y) to/from (%1,%2)").arg(uiStrings::sInline())
		.arg(uiStrings::sCrossline()), CallBack() );
	*newutils += new uiSurvey::Util( "clipboard",
		od_static_tr("getUtils","Copy Survey Information to Clipboard"),
		CallBack() );

	utils.setIfNull(newutils,true);

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
    mAttachCB( inpsurveyfld_->selectionDone, uiNewSurveyByCopy::inpSel );
    inpsurveyfld_->setSurveyPath( curfnm );

    newsurveyfld_ = new uiSurveySelect( this, false, false, "New Survey name" );
    newsurveyfld_->attach( alignedBelow,  inpsurveyfld_ );
    inpSel(nullptr);
}


~uiNewSurveyByCopy()
{
    detachAllNotifiers();
}


void inpSel( CallBacker* )
{
    BufferString fullpath;
    inpsurveyfld_->getFullSurveyPath( fullpath );
    fullpath.add( "_copy" );

    SurveyDiskLocation sdl;
    sdl.set( fullpath );
    newsurveyfld_->setSurveyDiskLocation( sdl );
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

bool acceptOK( CallBacker* ) override
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



// uiStartNewSurveySetup
uiStartNewSurveySetup::uiStartNewSurveySetup(uiParent* p, const char* dataroot,
					      SurveyInfo& survinfo )
    : uiDialog(p,Setup(tr("Create New Survey"),
		       tr("Specify new survey parameters"),
		       mODHelpKey(mStartNewSurveySetupHelpID)))
    , survinfo_(survinfo)
    , dataroot_(dataroot)
    , sips_(uiSurveyInfoEditor::survInfoProvs())
{
    setOkText( uiStrings::sNext() );

    survnmfld_ = new uiGenInput( this, tr("Survey name") );
    survnmfld_->setElemSzPol( uiObject::Wide );
    survnmfld_->setDefaultTextValidator();

    for ( int idx=0; idx<sips_.size(); idx++ )
    {
	if ( !sips_[idx]->isAvailable() )
	{
	    sips_.removeSingle( idx );
	    idx--;
	}
    }

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Initial setup") );
    sipfld_ = new uiListBox( this, su );
    sipfld_->attach( alignedBelow, survnmfld_ );

    zistimefld_ = new uiButtonGroup( this, "Z Domain", OD::Horizontal );
    new uiLabel( this, tr("Z Domain"), zistimefld_ );
    new uiRadioButton( zistimefld_, uiStrings::sTime() );
    new uiRadioButton( zistimefld_, uiStrings::sDepth() );
    new uiRadioButton( zistimefld_, tr("I don't know yet") );
    zistimefld_->selectButton( 0 );
    mAttachCB( zistimefld_->valueChanged, uiStartNewSurveySetup::zdomainChg );
    zistimefld_->attach( alignedBelow, sipfld_ );

    zinfeetfld_ = new uiGenInput( this, tr("Depth unit"),
				BoolInpSpec(true,tr("Meter"),tr("Feet")) );
    zinfeetfld_->attach( alignedBelow, zistimefld_ );
    zinfeetfld_->display( !isTime() );

    fillSipsFld();
}


uiStartNewSurveySetup::~uiStartNewSurveySetup()
{
    detachAllNotifiers();
}


void uiStartNewSurveySetup::setSurveyNameFld( const char* name, bool canedit )
{
    survnmfld_->setText( name );
    survnmfld_->setSensitive( canedit );
}


bool uiStartNewSurveySetup::isOK()
{
    BufferString survnm = survName();
    if ( survnm.isEmpty() )
	mErrRet( tr("Please enter a name for this new survey") )

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


bool uiStartNewSurveySetup::acceptOK( CallBacker* )
{
    if ( !isOK() )
	return false;

    const BufferString survnm = survName();
    survinfo_.setName( survnm );
    survinfo_.updateDirName();
    survinfo_.setZUnit( isTime(), isInFeet() );
    survinfo_.setSipName( sipName() );

    return true;
}


BufferString uiStartNewSurveySetup::sipName() const
{
    const int sipidx = sipfld_->currentItem();
    return sips_.validIdx(sipidx) ? sips_[sipidx]->usrText() : "";
}


void uiStartNewSurveySetup::fillSipsFld()
{
    int preferredsel = sipfld_->isEmpty() ? -1 : sipfld_->currentItem();
    sipfld_->setEmpty();

    for ( auto* sip : sips_ )
    {
	const bool issegysip = BufferString( sip->usrText() ).find( "SEG-Y" );
	if ( !issegysip )
	    continue;

	sips_ -= sip;
	sips_.insertAt( sip, 0 );
	preferredsel = 0;
	break;
    }

    const int nrprovs = sips_.size();
    for ( int idx=0; idx<nrprovs; idx++ )
    {
	uiSurvInfoProvider& sip = *sips_[idx];
	if ( preferredsel < 0 )
	{
	    if ( StringView(sip.usrText()).contains("etrel") )
		preferredsel = idx;
	}

	sipfld_->addItem( toUiString(sip.usrText()) );
	const char* icnm = sip.iconName();
	if ( !icnm || !*icnm )
	    icnm = "empty";
	sipfld_->setIcon( idx, icnm );
    }

    sipfld_->addItem( tr("Enter by hand") ); // always last
    sipfld_->setIcon( sipfld_->size()-1, "manualenter" );
    sipfld_->setCurrentItem( preferredsel < 0 ? 0 : preferredsel );
    sipfld_->resizeToContents();
}


BufferString uiStartNewSurveySetup::survName() const
{
    return survnmfld_->text();
}


bool uiStartNewSurveySetup::isTime() const
{
    return zistimefld_->selectedId() == 0;
}


bool uiStartNewSurveySetup::isDepth() const
{
    return zistimefld_->selectedId() == 1;
}


bool uiStartNewSurveySetup::isInFeet() const
{
    return !zinfeetfld_->getBoolValue();
}


void uiStartNewSurveySetup::zdomainChg( CallBacker* )
{
    zinfeetfld_->display( isDepth() );
}


//--- uiSurvey

uiSurvey::uiSurvey( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Survey Setup and Selection"),
				 mNoDlgTitle,mODHelpKey(mSurveyHelpID)))
    , orgdataroot_(GetBaseDataDir())
    , dataroot_(GetBaseDataDir())
    , initialsurveyname_(GetSurveyName())
{
    setVideoKey( mODVideoKey(mSurveyHelpID) );

    if ( dataroot_.isEmpty() )
    {
	new uiLabel( this,
		tr("Cannot establish a 'Survey Data Root' folder."
		"\nOpendTect needs a place to store its files."
		"\nPlease consult the documentation at dgbes.com,"
		"\nor contact support@dgbes.com.") );
    }

    setCurrentSurvInfo( new SurveyInfo(SI()) );

    auto* topgrp = new uiGroup( this, "Top Group" );

    datarootsel_ = new uiDataRootSel( topgrp );
    mAttachCB( datarootsel_->selectionChanged,uiSurvey::dataRootChgCB );

    auto* settbut = new uiToolButton( topgrp, "settings",
				      tr("General Settings"),
				      mCB(this,uiSurvey,odSettsButPush) );
    settbut->attach( rightOf, datarootsel_ );

    auto* sep1 = new uiSeparator( topgrp, "Separator 1" );
    sep1->attach( stretchedBelow, datarootsel_ );

    auto* leftgrp = new uiGroup( topgrp, "Survey selection left" );
    auto* rightgrp = new uiGroup( topgrp, "Survey selection right" );

    fillLeftGroup( leftgrp );
    fillRightGroup( rightgrp );
    leftgrp->attach( ensureBelow, sep1 );
    rightgrp->attach( rightOf, leftgrp );

    auto* tabs = new uiTabStack( this, "Info tabs" );

    auto* infogrp = new uiGroup( tabs->tabGroup(), "Info Group" );
    infofld_ = new uiTextEdit( infogrp, "Info", true );
    infofld_->setPrefHeightInChar( 10 );
    infofld_->setStretch( 2, 2 );
    tabs->addTab( infogrp, uiStrings::sInformation(), "info" );

    auto* notesgrp = new uiGroup( tabs->tabGroup(), "Notes Group" );
    notesfld_ = new uiTextEdit( notesgrp, "Survey Notes" );
    notesfld_->setPrefHeightInChar( 10 );
    notesfld_->setStretch( 2, 2 );
    tabs->addTab( notesgrp, tr("Notes"), "notes" );

    auto* loggrp = new uiGroup( tabs->tabGroup(), "Log Group" );
    logfld_ = new uiTextEdit( loggrp, "Survey Log", true );
    logfld_->setPrefHeightInChar( 10 );
    logfld_->setStretch( 2, 2 );
    tabs->addTab( loggrp, tr("Log"), "logfile" );

    auto* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( tabs );

    putToScreen();
    setOkText( uiStrings::sSelect() );
    mAttachCB( postFinalize(), uiSurvey::selChange );
}


uiSurvey::~uiSurvey()
{
    detachAllNotifiers();
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


static void osrbuttonCB( CallBacker* )
{
    uiDesktopServices::openUrl( "https://opendtect.org/osr" );
}


void uiSurvey::fillLeftGroup( uiGroup* grp )
{
    dirfld_ = new uiListBox( grp, "Surveys" );
    updateSurvList();
    mAttachCB( dirfld_->selectionChanged, uiSurvey::selChange );
    mAttachCB( dirfld_->doubleClicked, uiSurvey::accept );
    dirfld_->setHSzPol( uiObject::WideVar );
    dirfld_->setStretch( 2, 2 );

    auto* butgrp = new uiButtonGroup( grp, "Buttons", OD::Vertical );
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


static void copyFolderIconIfMissing( const char* basedir, const char* survdir )
{
    const FilePath survfp( basedir, survdir );
    const FilePath fp( mGetSWDirDataDir(), SurveyInfo::sKeyBasicSurveyName() );
    FilePath dest( survfp, "desktop.ini" );
    if ( !dest.exists() )
    {
	const FilePath src( fp, "desktop.ini" );
	if ( src.exists() )
	    File::copy( src.fullPath(), dest.fullPath() );
    }

    if ( dest.exists() )
	File::hide( dest.fullPath(), true );

    dest.set( survfp.fullPath() ).add( "OD.ico" );
    if ( !dest.exists() )
    {
	const FilePath src( fp, "OD.ico" );
	if ( src.exists() )
	    File::copy( src.fullPath(), dest.fullPath() );
    }

    File::setSystemFileAttrib( survfp.fullPath(), true );
}




void uiSurvey::updateSurveyNames()
{
    surveynames_.erase(); surveydirs_.erase();

    BufferString basedir = dataroot_;
    if ( basedir.isEmpty() )
	basedir = GetBaseDataDir();

    DirList dl( basedir, File::DirsInDir );
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


void uiSurvey::updateDataRootInSettings()
{
    Settings::common().set( "Default DATA directory", dataroot_ );
    if ( !Settings::common().write() )
	uiMSG().warning( tr("Could not save Survey Data Root "
			    "location in the settings file") );
}


bool uiSurvey::acceptOK( CallBacker* )
{
    if ( !dirfld_ )
	return true;

    if ( dirfld_->isEmpty() )
	mErrRet(tr("Please create a survey (or press Cancel)"))

    const BufferString selsurv( selectedSurveyName() );
    const bool samedataroot = dataroot_ == orgdataroot_;
    const bool samesurvey = samedataroot && initialsurveyname_ == selsurv;

    // Step 1: write local changes
    if ( !writeSurvInfoFileIfCommentChanged() )
	mErrRet(uiString::empty())
    if ( samedataroot && samesurvey && !parschanged_ )
    {
	uiRetVal uirv;
	if ( !IOMan::isOK() )
	    IOMan::setDataSource( dataroot_.buf(), selsurv.buf(), false );

	return uirv.isOK();
    }

    if ( !checkSurveyName() )
	mErrRet(uiString::empty())

    // Step 2: write default/current survey file and record data root preference
    uiRetVal uirv;
    const SurveyDiskLocation sdl( selectedSurveyName(), dataroot_ );
    if ( !IOMan::recordDataSource(sdl,uirv) )
    {
	if ( !uirv.isOK() )
	     uiMSG().error( uirv );
	return false;
    }

    // Step 3: Do the IOMan changes necessary
    if ( samesurvey )
    {
	if ( cursurvinfo_ )
	    eSI() = *cursurvinfo_;

	IOM().surveyParsChanged();
    }
    else
    {
	if ( !samedataroot && !uiSetDataDir::setRootDataDir(this,dataroot_) )
	    return false;

	const bool newdr = !samedataroot || initialsurveyname_.isEmpty() ||
			   !IOMan::isOK();
	if ( (newdr && IOMan::setDataSource(sdl.fullPath()).isOK()) ||
	     (!newdr && IOMan::newSurvey(cursurvinfo_).isOK()) )
	{
	    if ( newdr )
		deleteAndNullPtr( cursurvinfo_ );
	    else
		cursurvinfo_ = nullptr; // it's not ours anymore

	    if ( survmap_ )
		survmap_->setSurveyInfo( nullptr );
	}
	else
	{
	    const SurveyInfo* si = &SI();
	    if ( cursurvinfo_ == si )
	    {
		cursurvinfo_ = nullptr;
		if ( survmap_ )
		    survmap_->setSurveyInfo( nullptr );
	    }

	    const bool isblocked = IOM().message().isEmpty();
	    if ( !isblocked )
		uiMSG().error( mToUiStringTodo(IOM().message()) );
	    return false;
	}
    }

    // Step 4: if fresh survey, help user on his/her way
    if ( impiop_ && impsip_ )
    {
	freshsurveyselected_ = true;
	readSurvInfoFromFile();
	const char* askq = impsip_->importAskQuestion();
	if ( askq && *askq && uiMSG().askGoOn(mToUiStringTodo(askq)) )
	{
	    IOM().to( IOObjContext::Seis );
	    impsip_->startImport( this, *impiop_ );
	}
    }

    return true;
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

    return true;
}


bool uiSurvey::hasSurveys() const
{
    return dirfld_ && !dirfld_->isEmpty();
}


void uiSurvey::setCurrentSurvInfo( SurveyInfo* newsi, bool updscreen )
{
    delete cursurvinfo_;
    cursurvinfo_ = newsi;

    if ( updscreen )
	putToScreen();
    else if ( survmap_ )
	survmap_->setSurveyInfo( nullptr );
}


void uiSurvey::rollbackNewSurvey( const uiString& errmsg )
{
    if ( !cursurvinfo_ )
	return;

    const FilePath fp( cursurvinfo_->diskLocation().fullPath() );
    const bool haverem = File::removeDir( fp.fullPath() );
    setCurrentSurvInfo( nullptr, false );
    readSurvInfoFromFile();
    if ( !errmsg.isEmpty()  )
    {
	const uiString tousr = haverem ? tr("New survey removed because:\n%1")
		.arg(mToUiStringTodo(errmsg))
		: tr("New survey folder is invalid because:\n%1")
		.arg(mToUiStringTodo(errmsg) );
	uiMSG().error( tousr );
    }
}


#define mRetRollBackNewSurvey(errmsg) \
{ \
    rollbackNewSurvey(errmsg); \
    selChange(nullptr); \
    return; \
}


void uiSurvey::newButPushed( CallBacker* )
{
    if ( !rootDirWritable() )
	return;

    const FilePath fp( mGetSWDirDataDir(), SurveyInfo::sKeyBasicSurveyName());
    PtrMan<SurveyInfo> newsurvinfo =
				    SurveyInfo::readDirectory( fp.fullPath() );
    if ( !newsurvinfo )
    {
	uiString errmsg = tr("Cannot read software default survey\n"
			     "Try to reinstall the OpendTect package");
	mErrRetVoid( errmsg )
    }

    uiStartNewSurveySetup dlg( this, dataroot_, *newsurvinfo );
    if ( !dlg.go() )
	return;

    const BufferString orgdirname = newsurvinfo->getDirName().buf();
    const BufferString storagedir = FilePath( dataroot_ ).add( orgdirname )
							    .fullPath();
    if ( !uiSurveyInfoEditor::copySurv(
		mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),0,
				  dataroot_,orgdirname) )
	mErrRetVoid( tr("Cannot make a copy of the default survey") );

    setCurrentSurvInfo( newsurvinfo.release(), false );
    cursurvinfo_->disklocation_.setBasePath( dataroot_ );
    File::setSystemFileAttrib( storagedir, true );
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

    uiString msg = tr("This will delete the entire survey folder:\n\t%1"
		      "\nFull path: %2").arg(selnm).arg(truedirnm);
    if ( !uiMSG().askDelete(msg) ) return;

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
	setCurrentSurvInfo( nullptr, true );

    selChange( nullptr );
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

    setCurrentSurvInfo( SurveyInfo::readDirectory(dlg.newdirnm_) );
    if ( !cursurvinfo_ )
	mErrRetVoid(tr("Could not read the copied survey"))

    cursurvinfo_->setName( FilePath(dlg.newdirnm_).fileName() );
    cursurvinfo_->updateDirName();
    if ( !cursurvinfo_->write() )
	uiMSG().warning( tr("Could not write updated survey info") );

    updateSurvList();
    dirfld_->setCurrentItem( dlg.newdirnm_.buf() );
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


void uiSurvey::dataRootChgCB( CallBacker* cb )
{
    dataroot_ = datarootsel_->getDataRoot();
    updateSurvList();
    if ( dirfld_->isEmpty() )
    {
	setCurrentSurvInfo( nullptr , true );
	return;
    }

    const char* ptr = GetSurveyName();
    if ( ptr && dirfld_->isPresent(ptr) )
	dirfld_->setCurrentItem( GetSurveyName() );

    selChange( nullptr );
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
    else if ( butidx==1 )
    {
	copyInfoToClipboard();
    }
    else
    {
	Util* util = getUtils()[butidx];
	util->cb_.doCall( this );
    }
}


static BufferString pointTxt( int idx, const BinID& bid, const Coord& crd )
{
    BufferString txt( "Corner ", idx, ":\t" );
    txt.add( "X: " ).add( crd.x, 2 ).add( "  Y: " ).add( crd.y, 2 );
    txt.add( "  IL: " ).add( bid.inl() ).add( "  XL: " ).add( bid.crl() );
    txt.addNewLine();
    return txt;
}

void uiSurvey::copyInfoToClipboard()
{
    BufferString txt;
    txt.add( "> Survey Information <\n" );
    infoset_.dumpPretty( txt );
    txt.addNewLine(2);

    const SurveyInfo& si = *cursurvinfo_;
    txt.add( "> Survey Coordinates <\n" );
    BinID bid = si.sampling(false).hsamp_.start_;
    txt.add( pointTxt(1,bid,si.transform(bid)) );
    bid.crl() = si.sampling(false).hsamp_.stop_.crl();
    txt.add( pointTxt(2,bid,si.transform(bid)) );
    bid = si.sampling(false).hsamp_.stop_;
    txt.add( pointTxt(3,bid,si.transform(bid)) );
    bid.crl() = si.sampling(false).hsamp_.start_.crl();
    txt.add( pointTxt(4,bid,si.transform(bid)) );

    const IOPar& logpars = si.logPars();
    if ( !logpars.isEmpty() )
    {
	txt.addNewLine();
	logpars.dumpPretty( txt );
    }

    uiClipboard::setText( txt.buf() );
    uiMSG().message( tr("Survey Information copied to clipboard") );
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


bool uiSurvey::checkSurveyName()
{
    if ( dirfld_->isEmpty() )
	{ pErrMsg( "No survey in the list" ); return false; }

    const BufferString seltxt( selectedSurveyName() );
    if ( seltxt.isEmpty() )
	mErrRet(tr("Survey folder name cannot be empty"))

    if ( !File::exists(FilePath(dataroot_,seltxt).fullPath()) )
	mErrRet(tr("Survey directory does not exist anymore"))

    return true;
}


void uiSurvey::readSurvInfoFromFile()
{
    const BufferString survnm( selectedSurveyName() );
    PtrMan<SurveyInfo> newsi;
    if ( !survnm.isEmpty() )
    {
	const BufferString fname = FilePath( dataroot_ )
			    .add( selectedSurveyName() ).fullPath();
	newsi = SurveyInfo::readDirectory( fname );
	if ( !newsi )
	    uiMSG().warning(
		    tr("Cannot read survey setup file: %1").arg(fname) );
    }

    if ( newsi )
	setCurrentSurvInfo( newsi.release() );
}


bool uiSurvey::doSurvInfoDialog( bool isnew )
{
    deleteAndNullPtr( impiop_ );
    impsip_ = nullptr;
    uiSurveyInfoEditor dlg( this, *cursurvinfo_, isnew );
    if ( isnew )
	cursurvinfo_ = nullptr; // dlg takes over cursurvinfo_

    if ( !dlg.isOK() )
	return false;

    mAttachCB( dlg.survParChanged, uiSurvey::updateInfo );
    if ( !dlg.go() )
    {
	if ( !isnew )
	    readSurvInfoFromFile();

	return false;
    }

    if ( initialsurveyname_ == selectedSurveyName() )
	parschanged_ = true;

    updateSurvList();
    dirfld_->setCurrentItem( dlg.dirName() );

    impiop_ = dlg.impiop_; dlg.impiop_ = nullptr;
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



static void sMakeLogParsPretty( IOPar& par, BufferString& txt, bool rmname )
{
    if ( rmname )
	par.setName( "" );

    par.replaceKey( sKey::Version(), "OpendTect Version" );
    par.replaceKey( sKey::CrBy(), "Created by" );
    par.replaceKey( sKey::CrFrom(), "Created from" );
    BufferString timestr = par.find( sKey::CrAt() );
    if ( !timestr.isEmpty() )
    {
	par.set( "Created at", Time::getLocalDateTimeFromString(timestr) );
	par.removeWithKey( sKey::CrAt().str() );
    }

    timestr = par.find( sKey::ModAt() );
    if ( !timestr.isEmpty() )
    {
	par.set( "Last Modified at", Time::getLocalDateTimeFromString(timestr));
	par.removeWithKey( sKey::ModAt().str() );
    }

    par.dumpPretty( txt );
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

    if ( !hassurveys || !cursurvinfo_ )
    {
	notesfld_->setText( uiString::emptyString() );
	infofld_->setText( uiString::emptyString() );
	logfld_->setText( uiString::emptyString() );
	return;
    }

    BufferString inlinfo;
    BufferString crlinfo;
    BufferString zkey, zinfo;
    BufferString bininfo;
    BufferString crsinfo;
    BufferString areainfo;
    BufferString survtypeinfo;
    BufferString orientinfo;
    BufferString locinfo;

    const SurveyInfo& si = *cursurvinfo_;
    areainfo.add( getAreaString(si.getArea(false),si.xyInFeet(),2,true) );
    notesfld_->setText( si.comment() );

    BufferString logtxt;
    const IOPar& logpars = si.logPars();
    if ( !logpars.isEmpty() )
    {
	IOPar logcp = logpars;
	sMakeLogParsPretty( logcp, logtxt, true );
    }
    else
	logtxt.set( "No log available" );

    logfld_->setText( logtxt );

    zkey.set( "Z range (" )
	.add( si.zIsTime() ? ZDomain::Time().unitStr()
			   : getDistUnitString(si.zInFeet(), false) )
	.add( ")" );

    if ( si.getCoordSystem() )
	crsinfo.add( si.getCoordSystem()->summary() );

    if ( si.sampling(false).hsamp_.totalNr() > 0 )
    {
	inlinfo.add( si.sampling(false).hsamp_.start_.inl() );
	inlinfo.add( " - ").add( si.sampling(false).hsamp_.stop_.inl() );
	inlinfo.add( " - " ).add( si.inlStep() );
	inlinfo.add( "; Total: ").add( si.sampling(false).nrInl() );
	crlinfo.add( si.sampling(false).hsamp_.start_.crl() );
	crlinfo.add( " - ").add( si.sampling(false).hsamp_.stop_.crl() );
	crlinfo.add( " - " ).add( si.crlStep() );
	crlinfo.add( "; Total: ").add( si.sampling(false).nrCrl() );

	const float inldist = si.inlDistance(), crldist = si.crlDistance();
	bininfo.add( inldist, 2 ).add( " / " ).add( crldist, 2 );
	bininfo.add( " (" ).add( si.getXYUnitString(false) )
	       .add( "/line)" );
    }

    StepInterval<float> sizrg( si.zRange(false) );
    sizrg.scale( si.zDomain().userFactor() );
    const int nrdec = si.nrZDecimals();
    zinfo.add( sizrg.start, nrdec ).add( " - " )
	 .add( sizrg.stop, nrdec ).add( " - " )
	 .add( sizrg.step, nrdec );
    zinfo.add( "; Total: ").add( sizrg.nrSteps()+1 );

    survtypeinfo.add( SurveyInfo::toString(si.survDataType()) );

    FilePath fp( si.diskLocation().fullPath() );
    fp.makeCanonical();
    locinfo.add( fp.fullPath() );

    const float usrang = Math::degFromNorth( si.angleXInl() );
    orientinfo.add( toString(usrang,2) ).add( " Degrees from N" );

    infoset_.setEmpty();
    infoset_.add( sKey::Name(), cursurvinfo_->name() );
    infoset_.add( "In-line range", inlinfo );
    infoset_.add( "Cross-line range", crlinfo );
    infoset_.add( zkey, zinfo );
    infoset_.add( "Inl/Crl bin size", bininfo );
    infoset_.add( "CRS", crsinfo );
    infoset_.add( "Area", areainfo );
    infoset_.add( "Survey type", survtypeinfo );
    infoset_.add( "In-line orientation", orientinfo );
    infoset_.add( "Location", locinfo );

    BufferString infostr;
    StringPairSet infoset = infoset_;
    infoset.remove( sKey::Name() );
    infoset.dumpPretty( infostr );
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


// static tools

extern int OD_Get_2D_Data_Conversion_Status();
extern void OD_Convert_2DLineSets_To_2DDataSets(uiString&,TaskRunner*);

bool uiSurvey::Convert_OD4_Data_To_OD5()
{
    OD::ModDeps().ensureLoaded( "Seis" );

    const int status = OD_Get_2D_Data_Conversion_Status();
    if ( status == 0 )
	return true;

    if ( status == 3 ) // Pre 4.2 surveys
    {
	uiString msg( tr( "The survey %1 appears to be too old. "
		"Please open this survey first in OpendTect 4.6 to update "
		"its database before using it in later versions of OpendTect." )
		.arg(SurveyInfo::curSurveyName()) );
	if ( uiMSG().askGoOn(msg,tr("Select another survey"),
			     uiStrings::sExitOD() ) )
	    return false;

	uiMain::instance().exit();
    }

    if ( status == 1 )
    {
	uiString msg( tr("The database of survey '%1' is still '4.6' or lower. "
		"It will now be converted. "
		"This may take some time depending on the amount of data. "
		"Note that after the conversion you will not be able to use "
		"this 2D data in 5.0 or older versions of OpendTect.")
		.arg(SurveyInfo::curSurveyName()) );

	const int res = uiMSG().question( msg, tr("Convert now"),
					  tr("Select another survey"),
					  uiStrings::sExitOD() );
	if ( res < 0 )
	    uiMain::instance().exit();

	if ( !res )
	{
	    uiMSG().message( tr("Please note that you can copy the survey "
				"using 'Copy Survey' tool in the "
				"'Survey Setup and Selection' window.") );
	    return false;
	}
    }

    uiString errmsg;
    if ( !Survey::GMAdmin().fetchFrom2DGeom(errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    uiTaskRunner taskrnr( uiMain::instance().topLevel(), false );
    OD_Convert_2DLineSets_To_2DDataSets( errmsg, &taskrnr );
    if ( !errmsg.isEmpty() )
    {
	uiMSG().error( errmsg );
	return false;
    }

    return true;
}


extern bool OD_Get_Body_Conversion_Status();
extern bool OD_Convert_Body_To_OD5(uiString&);

bool uiSurvey::Convert_OD4_Body_To_OD5()
{
    OD::ModDeps().ensureLoaded( "EarthModel" );

    const bool status = OD_Get_Body_Conversion_Status();
    if ( status == 0 )
	return true;

    const uiString msg( tr("OpendTect has a new geobody format. "
		"All the old geo-bodies of survey '%1' will now be converted. "
		"Note that after the conversion, you will still be able to use "
		"those geo-bodies in OpendTect 4.6.0, but only in patch p or "
		"later.").arg(SurveyInfo::curSurveyName()) );

    const int res = uiMSG().question( msg, tr("Convert now"),
					   tr("Do it later"),
					   uiStrings::sExitOD() );
    if ( res < 0 )
	uiMain::instance().exit();

    if ( !res )
    {
	uiMSG().message( tr("Please note that you will not be able to use "
			    "any of the old geo-bodies in this survey.") );
	return false;
    }

    uiString errmsg;
    if ( !OD_Convert_Body_To_OD5(errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    uiMSG().message( tr("All the geo-bodies have been converted!") );

    return true;
}


void uiSurvey::getSurveyList( BufferStringSet& list, const char* dataroot,
			      const char* excludenm )
{
    BufferString basedir = dataroot;
    if ( basedir.isEmpty() )
	basedir = GetBaseDataDir();

    DirList dl( basedir, File::DirsInDir );
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const BufferString& dirnm = dl.get( idx );
	if ( excludenm && dirnm == excludenm )
	    continue;

	const FilePath fp( basedir, dirnm, SurveyInfo::sKeySetupFileName() );
	if ( !File::exists(fp.fullPath()) )
	    continue;

	list.add( dirnm );
	copyFolderIconIfMissing( basedir, dirnm );
    }

    list.sort();
}


bool uiSurvey::survTypeOKForUser( bool is2d )
{
    const bool dowarn = (is2d && !SI().has2D()) || (!is2d && !SI().has3D());
    if ( !dowarn )
	return true;

    const uiString warnmsg = tr("Your survey is set up as '%1 data\n"
				"you will have to change the survey setup.\n\n"
				"Do you wish to continue?")
		  .arg(is2d ? tr("3-D only'.\nTo be able to actually use 2-D")
			    : tr("2-D only'.\nTo be able to actually use 3-D"));

    return uiMSG().askContinue( warnmsg );
}


bool uiSurvey::ensureValidDataRoot( uiRetVal& uirv, uiParent* p )
{
    while ( !IOMan::isValidDataRoot(GetBaseDataDir()).isOK() )
    {
	uiSetDataDir dlg( p );
	if ( !p )
	    dlg.setModal( true );

	if ( !dlg.go() )
	{
	    const BufferString appnm = ApplicationData::applicationName();
	    if ( uiMSG().askGoOn( tr("Without a valid Survey Data Root,\n"
				     "'%1' cannot start.\nDo you wish to exit?")
					.arg(appnm) ) )
	    {
		uirv.setOK();
		return false;
	    }
	}
	else if ( uiSetDataDir::setRootDataDir(p,dlg.selectedDir()) )
	    break;
    }

    uirv.setOK();
    return true;
}


uiSurvey::SurvSelState& uiSurvey::lastSurveyState()
{
    static uiSurvey::SurvSelState ret;
    return ret;
}


uiSurvey::SurvSelState uiSurvey::ensureValidSurveyDir( uiRetVal& uirv,
						       uiParent* p )
{
    if ( !p )
	p = uiMain::instance().topLevel();

    BufferString prevnm = GetDataDir();
    uiSurvey::SurvSelState& res = lastSurveyState();
    while ( true )
    {
	uiSurvey dlg( p );
	if ( !p )
	    dlg.setModal( true );

	if ( !dlg.go() )
	{
	    res = dlg.currentSurvRemoved() ? SurveyRemoved : InvalidSurvey;
	    return res;
	}
	else
	{
	    while ( true )
	    {
		if ( !Convert_OD4_Data_To_OD5() )
		{
		    Threads::sleep( 0.1 );
		    continue;
		}

		Convert_OD4_Body_To_OD5();
		break;
	    }

	    res = prevnm == GetDataDir() ? SameSurvey
		: dlg.freshSurveySelected() ? NewFresh : NewExisting;
	    return res;
	}
    }
}


bool uiSurvey::ensureGoodSurveySetup( uiRetVal& uirv, uiParent* p )
{
    if ( !p )
	p = uiMain::instance().topLevel();

    if ( !ensureValidDataRoot(uirv,p) )
	return false;

    BufferString errmsg;
    if ( IOMan::validSurveySetup(errmsg) )
    {
	while( true )
	{
	    if ( !Convert_OD4_Data_To_OD5() )
	    {
		Threads::sleep( 0.1 );
		continue;
	    }

	    Convert_OD4_Body_To_OD5();
	    break;
	}
	return true;
    }

    SurvSelState res = InvalidSurvey;
    const BufferString programname = ApplicationData::applicationName();
    if ( !IOMan::isOK() )
    {
	while ( res == InvalidSurvey )
	{
	    res = ensureValidSurveyDir( uirv, p );
	    if ( res == InvalidSurvey &&
		 uiMSG().askGoOn(od_static_tr("uiSurvey::ensureGoodSurveySetup",
				  "Without a valid survey, %1 "
				  "cannot start.\nDo you wish to exit?")
			.arg( programname )) )
		return false;
	}
    }

    return true;
}
