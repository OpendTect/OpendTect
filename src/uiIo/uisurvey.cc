/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uisurvey.h"

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

#include "survinfo.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "dirlist.h"
#include "envvars.h"
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


static const char*	sZipFileMask = "ZIP files (*.zip *.ZIP)";
#define mMapWidth	300
#define mMapHeight	300
#define mErrRetVoid(s)	{ if ( s ) uiMSG().error(s); return; }
#define mErrRet(s)	{ if ( s ) uiMSG().error(s); return false; }


//--- General tools


static ObjectSet<uiSurvey::Util>& getUtils()
{
    static ObjectSet<uiSurvey::Util>* utils = 0;
    if ( !utils )
    {
	utils = new ObjectSet<uiSurvey::Util>;
	*utils += new uiSurvey::Util( "xy2ic", "Convert (X,Y) to/from I/C",
				      CallBack() );
	*utils += new uiSurvey::Util( "spherewire",
				"Setup geographical coordinates",
				      CallBack() );
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

uiNewSurveyByCopy( uiParent* p, const char* dirnm )
	: uiDialog(p,uiDialog::Setup("Import survey from location",
		   "Copy surveys from any data root",mTODOHelpID))
{
    BufferString curfnm;
    if ( dirnm && *dirnm )
	curfnm = FilePath( GetBaseDataDir(), dirnm ).fullPath();
    else
	curfnm = GetBaseDataDir();

    inpsurveyfld_ = new uiSurveySelect( this,"Survey to copy" );
    inpsurveyfld_->setSurveyPath( curfnm );
    newsurveyfld_ = new uiSurveySelect( this, "New Survey name" );
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

    static const char* msg =
	"An unknown amount of data needs to be copied."
	"\nDuring the copy, OpendTect will freeze."
	"\nDepending on the data transfer rate, this can take a long time!"
	"\n\nDo you wish to continue?";
    if ( !uiMSG().askContinue( msg ) )
	return false;

    const BufferString fromdir = getTrueDir( inpdirnm_ );
    MouseCursorChanger cc( MouseCursor::Wait );
    if ( !File::copy( fromdir, newdirnm_ ) )
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

    BufferString	inpdirnm_;
    BufferString	newdirnm_;
    uiSurveySelect*	inpsurveyfld_;
    uiSurveySelect*	newsurveyfld_;

};


//--- uiStartNewSurveySetup


class uiStartNewSurveySetup : public uiDialog
{

public:
    			uiStartNewSurveySetup(uiParent*,SurveyInfo&);

    bool		isOK();
    bool		acceptOK(CallBacker*);

protected:

    SurveyInfo&		survinfo_;
    uiGenInput*		survnmfld_;
    uiCheckList*	pol2dfld_;
    uiComboBox*		sipfld_;
    uiCheckList*	zdomainfld_;
    uiGroup*		zunitgrp_;
    uiCheckList*	zunitfld_;
    ObjectSet<uiSurvInfoProvider>& sips_;

    BufferString	sipName() const;
    BufferString	survName() const { return survnmfld_->text(); }
    bool		has3D() const	 { return pol2dfld_->isChecked(0); }
    bool		has2D() const	 { return pol2dfld_->isChecked(1); }
    bool		isTime() const	 { return zdomainfld_->isChecked(0); }
    bool		isInFeet() const { return zunitfld_->isChecked(1); }

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
    zunitgrp_->display( !isTime() );
}

};


uiStartNewSurveySetup::uiStartNewSurveySetup( uiParent* p,
					      SurveyInfo& survinfo )
    	: uiDialog(p,uiDialog::Setup("Create new survey",
		    "Specify new survey parameters",mTODOHelpID))
	, survinfo_(survinfo)
	, sips_(uiSurveyInfoEditor::survInfoProvs())
{
    survnmfld_ = new uiGenInput( this, "Survey name" );
    survnmfld_->setElemSzPol( uiObject::Wide );

    uiGroup* pol2dgrp = new uiGroup( this, "Data type group" );
    uiLabel* pol2dlbl = new uiLabel( pol2dgrp, "Available data" );
    pol2dfld_ = new uiCheckList( pol2dgrp, "3D", "2D", uiCheckList::AtLeastOne);
    pol2dfld_->attach( rightOf, pol2dlbl );
    pol2dfld_->setChecked( 0, true );
    pol2dfld_->setChecked( 1, false );
    pol2dfld_->changed.notify( mCB(this,uiStartNewSurveySetup,pol2dChg) );
    pol2dgrp->setHAlignObj( pol2dfld_ );
    pol2dgrp->attach( alignedBelow, survnmfld_ );

    uiLabeledComboBox* siplcb = new uiLabeledComboBox( this, "Define by" );
    siplcb->attach( alignedBelow, pol2dgrp );
    sipfld_ = siplcb->box();

    uiGroup* zdomaingrp = new uiGroup( this, "Domain group" );
    uiLabel* domainlbl = new uiLabel( zdomaingrp, "Domain" );
    zdomainfld_ = new uiCheckList( zdomaingrp, "Time", "Depth",
	    			   uiCheckList::OneOnly );
    zdomainfld_->attach( rightOf, domainlbl );
    zdomainfld_->changed.notify( mCB(this,uiStartNewSurveySetup,zdomainChg) );
    zdomaingrp->setHAlignObj( zdomainfld_ );
    zdomaingrp->attach( alignedBelow, siplcb );

    zunitgrp_ = new uiGroup( this, "Z unit group" );
    uiLabel* zunitlbl = new uiLabel( zunitgrp_, "Depth unit" );
    zunitfld_ = new uiCheckList( zunitgrp_, "Meter", "Feet",
	    			 uiCheckList::OneOnly );
    zunitfld_->attach( rightOf, zunitlbl );
    zunitgrp_->setHAlignObj( zunitfld_ );
    zunitgrp_->attach( alignedBelow, zdomaingrp );
    zunitgrp_->display( false );

    setSip( false );
}


bool uiStartNewSurveySetup::isOK()
{
    BufferString survnm = survName();
    if ( survnm.isEmpty() )
    	mErrRet( "Please enter a new survey name" )

    char* str = survnm.buf();
    cleanupString( str, false, false, true );
    const BufferString storagedir = FilePath( GetBaseDataDir() ).add( str )
								.fullPath();
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
	const int len = strlen( sipfld_->textOfItem(idx) );
	if ( len > maxlen ) maxlen = len;
    }
    sipfld_->setPrefWidthInChar( maxlen + 5 );
}


//--- uiSurvey


uiSurvey::uiSurvey( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Survey selection",
	       "Select and setup survey","0.3.1").nrstatusflds(1))
    , initialdatadir_(GetBaseDataDir())
    , initialsurvey_(GetSurveyName())
    , cursurvinfo_(0)
    , survmap_(0)
    , dirfld_(0)
    , impiop_(0)
    , impsip_(0)
    , parschanged_(false)
    , cursurvremoved_(false)
{
    const int lbwidth = 250;
    const int noteshght = 5;
    const int totwdth = lbwidth + mMapWidth + 10;

    const char* ptr = GetBaseDataDir();
    if ( !ptr || !*ptr )
    {
	new uiLabel( this,
		"Cannot establish a base data directory."
		"\nOpendTect needs a place to store its files."
		"\nPlease consult the documentation at opendtect.org,"
		"\nor contact support@opendtect.org." );
    }

    setCurrentSurvInfo( new SurveyInfo(SI()) );

    static int sipidx2d mUnusedVar =
		uiSurveyInfoEditor::addInfoProvider( new ui2DSurvInfoProvider );
    static int sipidxcp mUnusedVar =
		uiSurveyInfoEditor::addInfoProvider( new uiCopySurveySIP );

    uiGroup* topgrp = new uiGroup( this, "TopGroup" );
    uiGroup* rightgrp = new uiGroup( topgrp, "Survey selection right" );

    survmap_ = new uiSurveyMap( rightgrp );
    survmap_->setPrefWidth( mMapWidth );
    survmap_->setPrefHeight( mMapHeight );

    uiGroup* leftgrp = new uiGroup( topgrp, "Survey selection left" );
    dirfld_ = new uiListBox( leftgrp, "Surveys" );
    updateSurvList();
    dirfld_->selectionChanged.notify( mCB(this,uiSurvey,selChange) );
    dirfld_->doubleClicked.notify( mCB(this,uiSurvey,accept) );
    dirfld_->setPrefWidth( lbwidth );
    dirfld_->setStretch( 2, 2 );
    leftgrp->attach( leftOf, rightgrp );

    uiPushButton* newbut = new uiPushButton( leftgrp, "&New",
	    			mCB(this,uiSurvey,newButPushed), false );
    newbut->attach( rightOf, dirfld_ );
    newbut->setPrefWidthInChar( 12 );
    rmbut_ = new uiPushButton( leftgrp, "&Remove",
	    		       mCB(this,uiSurvey,rmButPushed), false );
    rmbut_->attach( alignedBelow, newbut );
    rmbut_->setPrefWidthInChar( 12 );
    editbut_ = new uiPushButton( leftgrp, "&Edit",
	    			 mCB(this,uiSurvey,editButPushed), false );
    editbut_->attach( alignedBelow, rmbut_ );
    editbut_->setPrefWidthInChar( 12 );
    uiPushButton* copybut = new uiPushButton( leftgrp, "C&opy",
	    			 mCB(this,uiSurvey,copyButPushed), false );
    copybut->attach( alignedBelow, editbut_ );
    copybut->setPrefWidthInChar( 12 );

    uiToolButton* expbut = new uiToolButton( leftgrp, "share",
				    "Pack survey into a zip file",
				    mCB(this,uiSurvey,exportButPushed) );
    expbut->attach( alignedBelow, copybut );
    uiToolButton* impbut = new uiToolButton( leftgrp, "unpack",
				    "Unpack survey from zip file", 
				    mCB(this,uiSurvey,importButPushed) );
    impbut->attach( rightAlignedBelow, copybut );

    ObjectSet<uiSurvey::Util>& utils = getUtils();
    uiGroup* utilbutgrp = new uiGroup( rightgrp, "Surv Util buttons" );
    const CallBack cb( mCB(this,uiSurvey,utilButPush) );
    for ( int idx=0; idx<utils.size(); idx++ )
    {
	const uiSurvey::Util& util = *utils[idx];
	uiToolButton* but = new uiToolButton( utilbutgrp, util.pixmap_,
						util.tooltip_, cb );
	but->setToolTip( util.tooltip_ );
	utilbuts_ += but;
	if ( idx > 0 )
	    utilbuts_[idx]->attach( rightOf, utilbuts_[idx-1] );
    }
    utilbutgrp->attach( centeredBelow, survmap_ );

    uiPushButton* drbut = new uiPushButton( leftgrp, "&Set Data Root",
	    			mCB(this,uiSurvey,dataRootPushed), false );
    drbut->attach( centeredBelow, dirfld_ );

    uiSeparator* horsep1 = new uiSeparator( topgrp );
    horsep1->setPrefWidth( totwdth );
    horsep1->attach( stretchedBelow, rightgrp, -2 );
    horsep1->attach( ensureBelow, leftgrp );

    uiGroup* infoleft = new uiGroup( topgrp, "Survey info left" );
    uiGroup* inforight = new uiGroup( topgrp, "Survey info right" );
    infoleft->attach( alignedBelow, leftgrp );
    infoleft->attach( ensureBelow, horsep1 );
    inforight->attach( alignedBelow, rightgrp );
    inforight->attach( ensureBelow, horsep1 );

    infoleft->setFont( FontList().get(FontData::key(FontData::ControlSmall)) );
    inforight->setFont( FontList().get(FontData::key(FontData::ControlSmall)));

    inllbl_ = new uiLabel( infoleft, "" ); 
    crllbl_ = new uiLabel( infoleft, "" );
    arealbl_ = new uiLabel( infoleft, "" );
    zlbl_ = new uiLabel( inforight, "" ); 
    binlbl_ = new uiLabel( inforight, "" );
    typelbl_ = new uiLabel( inforight, "" );
    inllbl_->setPrefWidthInChar( 40 );
    crllbl_->setPrefWidthInChar( 40 );
    zlbl_->setPrefWidthInChar( 40 );
    binlbl_->setPrefWidthInChar( 40 );
    arealbl_->setPrefWidthInChar( 40 );
    typelbl_->setPrefWidthInChar( 40 );

    crllbl_->attach( alignedBelow, inllbl_ );
    arealbl_->attach( alignedBelow, crllbl_ );
    binlbl_->attach( alignedBelow, zlbl_ );
    typelbl_->attach( alignedBelow, binlbl_ );

    uiGroup* botgrp = new uiGroup( this, "Bottom Group" );
    uiLabel* notelbl = new uiLabel( botgrp, "Notes:" );
    notes_ = new uiTextEdit( botgrp, "Notes" );
    notes_->attach( alignedBelow, notelbl );
    notes_->setPrefHeightInChar( noteshght );
    notes_->setPrefWidth( totwdth );
    notes_->setStretch( 2, 2 );

    uiSplitter* splitter = new uiSplitter( this, "Splitter", false );
    splitter->addGroup( topgrp );
    splitter->addGroup( botgrp );

    putToScreen();
    setOkText( "&Ok (Select)" );
}


uiSurvey::~uiSurvey()
{
    delete impiop_;
    delete cursurvinfo_;
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
    const char* rootdir = GetBaseDataDir();
    if ( !File::isWritable(rootdir) )
    {
	BufferString msg( "Cannot create new survey in\n",rootdir, \
			  ".\nDirectory is write protected."); \
	uiMSG().error( msg ); \
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
    Settings::common().set( "Default DATA directory", GetBaseDataDir() );
    if ( !Settings::common().write() )
	uiMSG().warning(
		"Could not save the base data location in the settings file" );
}


bool uiSurvey::acceptOK( CallBacker* )
{
    if ( !dirfld_ )
	return true;
    if ( dirfld_->isEmpty() )
	mErrRet("Please create a survey (or press Cancel)")

    const BufferString selsurv( selectedSurveyName() );
    const bool samedataroot = initialdatadir_ == GetBaseDataDir();
    const bool samesurvey = samedataroot && initialsurvey_ == selsurv;
    if ( samedataroot && samesurvey && !parschanged_ )
	return true;

    // Step 1: write local changes
    if ( !writeSurvInfoFileIfCommentChanged() )
	mErrRet(0)

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
	if ( IOMan::newSurvey(cursurvinfo_) )
	    cursurvinfo_ = 0; // it's not ours anymore
	else
	{
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

    return true;
}


bool uiSurvey::rejectOK( CallBacker* )
{
    if ( cursurvremoved_ )
	mErrRet( "You have removed the current survey ...\n"
		   "You *have to* select a survey now!" )

    if ( initialdatadir_ != GetBaseDataDir() )
    {
	if ( !uiSetDataDir::setRootDataDir(this,initialdatadir_) )
	    mErrRet( "As we cannot reset to the old Data Root,\n"
		       "You *have to* select a survey now!" )
    }

    return true;
}


void uiSurvey::setCurrentSurvInfo( SurveyInfo* newsi, bool updscreen )
{
    delete cursurvinfo_; cursurvinfo_ = newsi;
    if ( updscreen )
	putToScreen();
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

    FilePath fp( GetSoftwareDir(0), "data", SurveyInfo::sKeyBasicSurveyName() );
    SurveyInfo* newsurvinfo = SurveyInfo::read( fp.fullPath() );
    if ( !newsurvinfo )
    {
	BufferString errmsg( "Cannot read software default survey\n" );
	errmsg.add( "Try to reinstall the OpendTect package" );
	mErrRetVoid( errmsg )
    }

    uiStartNewSurveySetup dlg( this, *newsurvinfo );
    if ( !dlg.go() )
	{ delete newsurvinfo; return; }

    const BufferString rootdir( GetBaseDataDir() );
    const BufferString orgdirname = newsurvinfo->getDirName().buf();
    const BufferString storagedir = FilePath( rootdir ).add( orgdirname )
						       	    .fullPath();
    if ( !uiSurveyInfoEditor::copySurv(
		mGetSetupFileName(SurveyInfo::sKeyBasicSurveyName()),0,
				  rootdir,orgdirname) )
	{ delete newsurvinfo;
	    mErrRetVoid( "Cannot make a copy of the default survey" ); }

    setCurrentSurvInfo( newsurvinfo, false );

    cursurvinfo_->datadir_ = rootdir;
    if ( !File::makeWritable(storagedir,true,true) )
	mRetRollBackNewSurvey("Cannot set the permissions for the new survey")

    if ( !cursurvinfo_->write(rootdir) )
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
    const BufferString seldirnm = FilePath(GetBaseDataDir())
					    .add(selnm).fullPath();
    const BufferString truedirnm = getTrueDir( seldirnm );

    BufferString msg( "This will remove the entire survey directory:\n\t" );
    msg += selnm;
    msg += "\nFull path: "; msg += truedirnm;
    if ( !uiMSG().askRemove( msg ) ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );
    const bool rmisok = File::remove( truedirnm );
    MouseCursorManager::restoreOverride();
    if ( !rmisok )
	uiMSG().error( BufferString( truedirnm, "\nnot removed properly" ) );

    if ( seldirnm != truedirnm ) // must have been a link
	if ( !File::remove(seldirnm) )
	    uiMSG().error( "Could not remove link to the removed survey" );

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

    uiNewSurveyByCopy dlg( this, selectedSurveyName() );
    if ( !dlg.go() )
	return;

    setCurrentSurvInfo( SurveyInfo::read(dlg.newdirnm_) );
    if ( !cursurvinfo_ )
	mErrRetVoid( "Could not read the copied survey" )

    cursurvinfo_->setName( FilePath(dlg.newdirnm_).fileName() );
    cursurvinfo_->updateDirName();
    if ( !cursurvinfo_->write() )
	uiMSG().warning( "Could not write updated survey info" );

    updateSurvList();
    dirfld_->setCurrentItem( dlg.newdirnm_ );
}


void uiSurvey::importButPushed( CallBacker* )
{
    if ( !rootDirWritable() ) return;

    uiFileDialog fdlg( this, true, 0, "*.zip", "Select survey zip file" );
    fdlg.setSelectedFilter( sZipFileMask );
    if ( !fdlg.go() )
	return;

    uiSurvey_UnzipFile( this, fdlg.fileName(), GetBaseDataDir() );
    updateSurvList();
    //TODO set unpacked survey as current with dirfld_->setCurrentItem()
}


static void osrbuttonCB( void* )
{
    uiDesktopServices::openUrl( "https://opendtect.org/osr" );
}


void uiSurvey::exportButPushed( CallBacker* )
{
    const BufferString survnm( selectedSurveyName() );
    const BufferString title( "Pack ", survnm, " survey into zip file" );
    uiDialog dlg( this,
    uiDialog::Setup(title,mNoDlgTitle,mTODOHelpID));
    uiFileInput* filepinput = new uiFileInput( &dlg,"Select output destination",
		    uiFileInput::Setup().directories(false).forread(false)
		    .allowallextensions(false));
    filepinput->setFilter( sZipFileMask );
    uiLabel* sharfld = new uiLabel( &dlg, 
			   "You can share surveys to Open Seismic Repository."
			   "To know more " );
    sharfld->attach( leftAlignedBelow,  filepinput );
    uiPushButton* osrbutton = new uiPushButton( &dlg, 
				    "Click here", mSCB(osrbuttonCB), false );
    osrbutton->attach( rightOf, sharfld );
    if ( !dlg.go() )
	return;
	
    FilePath exportzippath( filepinput->fileName() );
    BufferString zipext = exportzippath.extension();
    if ( zipext != "zip" )
	mErrRetVoid( "Please add .zip extension to the file name" )

    uiSurvey_ZipDirectory( this, survnm, exportzippath.fullPath() );
}


void uiSurvey::dataRootPushed( CallBacker* )
{
    uiSetDataDir dlg( this );
    if ( !dlg.go() )
	return;

    updateSurvList();
    const char* ptr = GetSurveyName();
    if ( ptr && dirfld_->isPresent(ptr) )
	dirfld_->setCurrentItem( GetSurveyName() );
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



void uiSurvey::updateSurvList()
{
    NotifyStopper ns( dirfld_->selectionChanged );
    const BufferString prevsel( dirfld_->getText() );
    dirfld_->setEmpty();
    BufferStringSet dirlist; getSurveyList( dirlist );
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

    if ( !File::exists(FilePath(GetBaseDataDir(),seltxt).fullPath()) )
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
	const BufferString fname = FilePath( GetBaseDataDir() )
			    .add( selectedSurveyName() ).fullPath();
	newsi = SurveyInfo::read( fname );
	if ( !newsi )
	    uiMSG().warning(
		    BufferString("Cannot read survey setup file: ",fname) );
    }
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

    if ( initialsurvey_ == selectedSurveyName() )
	parschanged_ = true;

    updateSurvList();
    dirfld_->setCurrentItem( dlg.dirName() );

    impiop_ = dlg.impiop_; dlg.impiop_ = 0;
    impsip_ = dlg.lastsip_;
    return true;
}


void uiSurvey::selChange( CallBacker* )
{
    writeSurvInfoFileIfCommentChanged();
    readSurvInfoFromFile();
    putToScreen();
}



void uiSurvey::putToScreen()
{
    if ( !survmap_ ) return;

    survmap_->setSurveyInfo( cursurvinfo_ );

    BufferString inlinfo( "In-line range: " );
    BufferString crlinfo( "Cross-line range: " );
    BufferString survtypeinfo( "Survey type: " );
    BufferString zinfo( "Z range" );
    BufferString bininfo( "Bin size" );
    BufferString areainfo( "Area" );

    if ( !cursurvinfo_ )
    {
	notes_->setText( "" );
	zinfo.add( ":" ); bininfo.add( ":" ); areainfo.add( ":" );
    }
    else
    {
	const SurveyInfo& si = *cursurvinfo_;
	notes_->setText( si.comment() );

	zinfo.add( " (" ).add( si.getZUnitString(false) ).add( "): ");
	bininfo.add( " (" ).add( si.getXYUnitString(false) ).add( "/line): " );
	areainfo.add( " (sq " ).add( si.xyInFeet() ? "mi" : "km" ).add( "): " );

	if ( si.sampling(false).hrg.totalNr() > 0 )
	{
	    inlinfo.add( si.sampling(false).hrg.start.inl() );
	    inlinfo.add( " - ").add( si.sampling(false).hrg.stop.inl() );
	    inlinfo.add( "  step: " ).add( si.inlStep() );
	    crlinfo.add( si.sampling(false).hrg.start.crl() );
	    crlinfo.add( " - ").add( si.sampling(false).hrg.stop.crl() );
	    crlinfo.add( "  step: " ).add( si.crlStep() );
	    
	    const float inldist = si.inlDistance(), crldist = si.crlDistance();
	    #define mAddDist(dist) \
	    nr = (int)(dist*100+.5); bininfo += nr/100; \
	    rest = nr%100; bininfo += rest < 10 ? ".0" : "."; bininfo += rest

	    int nr, rest;    
	    bininfo += "inl: "; mAddDist(inldist);
	    bininfo += "  crl: "; mAddDist(crldist);
	    float area = (float) ( si.computeArea(false) * 1e-6 ); //in km2
	    if ( si.xyInFeet() )
		area /= 2.590; // square miles

	    areainfo += area;
	}

	#define mAdd2ZString(nr) zinfo += istime ? mNINT32(1000*nr) : nr;

	const bool istime = si.zIsTime(); 
	mAdd2ZString( si.zRange(false).start );
	zinfo += " - "; mAdd2ZString( si.zRange(false).stop );
	zinfo += "  step: "; mAdd2ZString( si.zRange(false).step );
	survtypeinfo.add( ": " ).add( SurveyInfo::toString(si.survDataType()) );
    }
    inllbl_->setText( inlinfo );
    crllbl_->setText( crlinfo );
    binlbl_->setText( bininfo );
    arealbl_->setText( areainfo );
    zlbl_->setText( zinfo );
    typelbl_->setText( survtypeinfo );

    const bool anysvy = !dirfld_->isEmpty();
    rmbut_->setSensitive( anysvy );
    editbut_->setSensitive( anysvy );
    for ( int idx=0; idx<utilbuts_.size(); idx++ )
	utilbuts_[idx]->setSensitive( anysvy );

    if ( cursurvinfo_ )
    {
	FilePath fp( cursurvinfo_->datadir_, cursurvinfo_->dirname_ );
	fp.makeCanonical();
	toStatusBar( fp.fullPath() );
    }
}


bool uiSurvey::writeSurvInfoFileIfCommentChanged()
{
    if ( !cursurvinfo_ || !notes_->isModified() )
	return true;

    cursurvinfo_->setComment( notes_->text() );
    if ( !cursurvinfo_->write( GetBaseDataDir() ) )
	mErrRet( "Failed to write survey info.\nNo changes committed." )

    return true;
}
