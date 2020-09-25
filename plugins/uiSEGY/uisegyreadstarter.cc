/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/

#include "uisegyreadstarter.h"

#include "uisegyreadstartinfo.h"
#include "uisegyreadfinisher.h"
#include "uisegyimpparsdlg.h"
#include "uisegyimptype.h"
#include "uisegyexamine.h"
#include "uisegymanip.h"
#include "uisegysipclassic.h"
#include "uisegydef.h"
#include "uisegyread.h"
#include "segyvintageimporter.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uiseparator.h"
#include "uisurvmap.h"
#include "uihistogramdisplay.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uispinbox.h"
#include "uilineedit.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uisplitter.h"
#include "segyhdr.h"
#include "segyhdrkeydata.h"
#include "seisinfo.h"
#include "posinfodetector.h"
#include "dataclipper.h"
#include "filepath.h"
#include "fileformat.h"
#include "cubesampling.h"
#include "dirlist.h"
#include "oddirs.h"
#include "genc.h"
#include "survinfo.h"
#include "od_istream.h"
#include "settings.h"
#include "timer.h"
#include "trckeyzsampling.h"
#include "repos.h"
#include "iopar.h"
#include "keystrs.h"


#define mForSurvSetup su.forsurvsetup_
#define mSurvMapHeight 300
#define mDefSize 250
#define mClipSamplerBufSz 100000

static const char* sKeyClipRatio = "Amplitudes.Clip Ratio";
static const char* sKeyIncludeZeros = "Amplitudes.Include Zeros";
static BufferString sImportFromPath = GetDataDir();

uiSEGYReadStarter::uiSEGYReadStarter( uiParent* p, const Setup& su )
    : uiDialog(p,uiDialog::Setup(su.forsurvsetup_
	    ? tr("Extract Survey Setup from SEG-Y") : tr("Import SEG-Y Data"),
            mNoDlgTitle, mODHelpKey(mSEGYReadStarterHelpID)).nrstatusflds(1))
    , filereadopts_(0)
    , typfld_(0)
    , useicbut_(0)
    , usexybut_(0)
    , keepzsampbox_(0)
    , coordscalefld_(0)
    , ampldisp_(0)
    , survmap_(0)
    , detectrev0flds_(true)
    , userfilename_("_") // any non-empty non-existing
    , scaninfos_(0)
    , loaddef_(su.imptype_ ? su.imptype_->is2D() : false)
    , clipsampler_(*new DataClipSampler(100000))
    , lastscanwasfull_(false)
    , survinfo_(0)
    , survinfook_(false)
    , classicsip_(0)
    , timer_(0)
    , inpfld_(0)
    , setup_(su)
    , fullscanbut_(0)
    , editbut_(0)
{
    if ( mForSurvSetup )
	loaddef_.icvsxytype_ = SEGY::FileReadOpts::Both;
    else
    {
	setCtrlStyle( RunAndClose );
	setOkText( tr("Next >>") );

	loaddef_.icvsxytype_ = SEGY::FileReadOpts::ICOnly;
    }

    if ( setup_.vintagecheckmode_ )
	setCaption( tr("SEG-Y values and ranges") );

    topgrp_ = new uiGroup( this, "Top group" );
    uiLabel* selfilelbl = 0;
    uiObject* attachobj = 0;
    if ( setup_.fixedfnm_ )
    {
	selfilelbl = new uiLabel( topgrp_, toUiString(su.filenm_) );
	userfilename_.set( su.filenm_ );
	attachobj = selfilelbl;
    }
    else
    {
	uiFileSel::Setup fssu( OD::GeneralContent, filespec_.fileName() );
	fssu.objtype( tr("SEG-Y") )
	    .initialselectiondir( sImportFromPath )
	    .formats( uiSEGYFileSpec::fileFmts() );
	inpfld_ = new uiFileSel( topgrp_, uiStrings::sInputFile().postFixWord(
		     toUiString("*=%1").arg(tr("wildcard","usually a '*'")) ),
				 fssu );
	inpfld_->newSelection.notify( mCB(this,uiSEGYReadStarter,inpChg) );
	attachobj = inpfld_->attachObj();
    }

    if ( !setup_.vintagecheckmode_ )
    {
	editbut_ = uiButton::getStd( topgrp_, OD::Edit,
				     mCB(this,uiSEGYReadStarter,editFile),
				     false );
	editbut_->attach( rightOf, attachobj );
	editbut_->setSensitive( false );
    }

    uiLabel* typlbl = 0;
    if ( su.imptype_ )
    {
	fixedimptype_ = *su.imptype_;
	typlbl = new uiLabel( topgrp_,
			tr( "Import %1" ).arg( su.imptype_->dispText() ) );
	typlbl->attach( ensureBelow, attachobj );
    }
    else
    {
	typfld_ = new uiSEGYImpType( topgrp_, !mForSurvSetup );
	typfld_->typeChanged.notify( mCB(this,uiSEGYReadStarter,typChg) );
	typfld_->attach( alignedBelow, attachobj );
    }

    zdombox_ = new uiCheckBox( topgrp_, SI().zIsTime() ? uiStrings::sDepth()
						       : uiStrings::sTime() );
    if ( typfld_ )
	zdombox_->attach( rightOf, typfld_ );
    else
	zdombox_->attach( rightOf, typlbl );
    mAttachCB( zdombox_->activated, uiSEGYReadStarter::zDomChgCB );

    coordsysselfld_ = new Coords::uiCoordSystemSel( topgrp_ );
    coordsysselfld_->attach( rightOf, zdombox_ );
    mAttachCB( coordsysselfld_->butPush, uiSEGYReadStarter::coordSysChangedCB );
    coordsysselfld_->display( false );

    uiSeparator* sep = new uiSeparator( this, "Top sep" );
    sep->attach( stretchedBelow, topgrp_ );

    midgrp_ = new uiGroup( this, "Mid group" );
    midgrp_->attach( ensureBelow, sep );
    infofld_ = new uiSEGYReadStartInfo( midgrp_, loaddef_, su.imptype_ );
    infofld_->loaddefChanged.notify( mCB(this,uiSEGYReadStarter,defChg) );
    infofld_->revChanged.notify( mCB(this,uiSEGYReadStarter,revChg) );

    sep = new uiSeparator( this, "Bot sep" );
    sep->attach( stretchedBelow, midgrp_ );

    botgrp_ = new uiGroup( this, "Bottom group" );
    botgrp_->attach( ensureBelow, sep );
    uiGroup* survmapgrp = 0;
    if ( mForSurvSetup )
    {
	survmapgrp = new uiGroup( botgrp_, "SurvMap group" );
	survmap_ = new uiSurveyMap( survmapgrp, true );
	survmap_->setSurveyInfo( 0 );
	survmapgrp->setFrame( true );
    }
    uiGroup* amplgrp = createAmplDisp();
    if ( survmap_ )
    {
	uiSplitter* spl = new uiSplitter( botgrp_ );
	survmap_->view().setPrefWidth( mDefSize );
	spl->addGroup( survmapgrp );
	spl->addGroup( amplgrp );
	spl->attach( ensureBelow, infofld_ );
    }
    createTools();
    botgrp_->setStretch( 2, 1 );

    setToolStates();
    if ( setup_.vintagecheckmode_ && setup_.fixedfnm_ &&
	 !setup_.vintagenm_.isEmpty() )
	setCtrlStyle( CloseOnly );

    postFinalise().notify( mCB(this,uiSEGYReadStarter,initWin) );
}


#undef mForSurvSetup
#define mForSurvSetup survmap_


void uiSEGYReadStarter::createTools()
{
    uiGroup* toolgrp = new uiGroup( midgrp_, "Tool group" );
    toolgrp->setStretch( 0, 0 );
    toolgrp->attach( rightOf, infofld_ );
    uiToolButton* openbut = new uiToolButton( toolgrp, "open",
			setup_.vintagecheckmode_ ? tr("Select another vintage")
						 : tr("Use Saved SEG-Y setup"),
				mCB(this,uiSEGYReadStarter,readParsCB) );
    uiToolButton* savebut = new uiToolButton( toolgrp, "save",
			setup_.vintagecheckmode_ ? tr("Save this vintage")
					  : tr("Store this setup"),
				mCB(this,uiSEGYReadStarter,writeParsCB) );
    savebut->attach( rightOf, openbut );

    if ( setup_.vintagecheckmode_ )
    {
	const CallBack vntrefresh(mCB(this,uiSEGYReadStarter,vntRefreshCB));
	uiToolButton* resetbut = new uiToolButton( toolgrp, "undo",
					tr("Revert to auto detective vintage"),
					vntrefresh );
	resetbut->attach( rightOf, savebut );
    }

    fullscanbut_ = new uiToolButton( toolgrp, "fullscan",
				tr("Scan the entire input"),
				mCB(this,uiSEGYReadStarter,fullScanReq) );
    fullscanbut_->attach( alignedBelow, openbut );

    hdrentrysettsbut_ = new uiToolButton( toolgrp, "settings",
			    tr("Settings for byte location scanning"),
			    mCB(this,uiSEGYReadStarter,editHdrEntrySettings) );

    uiGroup* examinegrp = new uiGroup( toolgrp, "Examine group" );
    examinebut_ = new uiToolButton( examinegrp, "examine",
				    uiString::empty(),
				    mCB(this,uiSEGYReadStarter,examineCB) );
    examinenrtrcsfld_ = new uiSpinBox( examinegrp, 0, "Examine traces" );
    examinenrtrcsfld_->setInterval( 0, 1000000, 10 );
    examinenrtrcsfld_->setHSzPol( uiObject::Small );
    examinenrtrcsfld_->setToolTip( tr("Number of traces to examine") );
    examinenrtrcsfld_->attach( alignedBelow, examinebut_ );
    examinenrtrcsfld_->setInterval( 1, mUdf(int), 10 );
    examinenrtrcsfld_->setValue( uiSEGYExamine::Setup::getDefNrTrcs() );
    examinegrp->attach( alignedBelow, hdrentrysettsbut_ );
    examinegrp->setFrame( true );

    if ( imptypeFixed() && fixedimptype_.isVSP() )
	return;

    uiObject* lowest = examinegrp->attachObj();

    const bool imptypfixed = imptypeFixed();
    bool needicvsxy = !mForSurvSetup;
    if ( imptypfixed && fixedimptype_.is2D() )
	needicvsxy = false;
    if ( needicvsxy )
    {
	const CallBack icvsxycb( mCB(this,uiSEGYReadStarter,icxyCB) );
	useicbut_ = new uiRadioButton( toolgrp, tr("Use I/C"), icvsxycb );
	useicbut_->setToolTip(
	    tr("Select to use Inline/Crossline for positioning."
		"\nX and Y will be calculated using the survey setup.\nPrefer "
		"this if you have valid In- and Crossline numbers available."));
	useicbut_->attach( alignedBelow, lowest );

	usexybut_ = new uiRadioButton( toolgrp, tr("Use (X,Y)"), icvsxycb );
	usexybut_->setToolTip(
	    tr("Select to use (X,Y) for positioning.\nInline/Crossline "
		"will be calculated using the survey setup.\nOnly use this "
		"option if you do not have valid In- and Crossline numbers.") );
	usexybut_->attach( alignedBelow, useicbut_ );
	lowest = usexybut_;
    }

    if ( !imptypeFixed() || fixedimptype_.is2D() )
    {
	keepzsampbox_ = new uiCheckBox( toolgrp, tr("File Z's") );
	keepzsampbox_->setHSzPol( uiObject::Small );
	keepzsampbox_->setToolTip(
		tr("Use Z sampling as it appears in each SEG-Y file") );
	keepzsampbox_->setChecked( false );
	keepzsampbox_->activated.notify(
			mCB(this,uiSEGYReadStarter,keepZChg) );
	keepzsampbox_->attach( alignedBelow, lowest );
	lowest = keepzsampbox_;
    }

    coordscalefld_ = new uiLineEdit( toolgrp, FloatInpSpec(),"CoordScale");
    coordscalefld_->setHSzPol( uiObject::Small );
    coordscalefld_->setToolTip( tr("Enter a value if you want to ignore "
	"the coordinate scaling in the trace headers."
	"\nAll coordinates read will then be multiplied by that factor.") );
    coordscalefld_->setPlaceholderText( tr("scale XY") );
    coordscalefld_->editingFinished.notify(
			    mCB(this,uiSEGYReadStarter,coordscaleChg) );
    coordscalefld_->attach( alignedBelow, lowest );
    lowest = coordscalefld_;

    toolgrp->attach( rightOf, infofld_ );
}


uiGroup* uiSEGYReadStarter::createAmplDisp()
{
    uiGroup* amplgrp = new uiGroup( botgrp_, "Hist grp" );
    const CallBack adupcb( mCB(this,uiSEGYReadStarter,updateAmplDisplay) );

    uiHistogramDisplay::Setup hdsu;
    hdsu.noyaxis( false ).noygridline(true).annoty( false );
    hdsu.canvaswidth(mDefSize).canvasheight( mDefSize );
    ampldisp_ = new uiHistogramDisplay( amplgrp, hdsu );
    ampldisp_->setTitle( uiStrings::sAmplitude(mPlural) );

    clipfld_ = new uiSpinBox( amplgrp, 1, "Clipping percentage" );
    clipfld_->setInterval( 0.f, 49.9f, 0.1f );
    clipfld_->setValue( 0.1f );
    clipfld_->setToolTip( tr("Percentage clip for display") );
    clipfld_->setSuffix( toUiString("%") );
    clipfld_->setHSzPol( uiObject::Small );
    clipfld_->attach( rightOf, ampldisp_ );
    clipfld_->valueChanging.notify( adupcb );

    inc0sbox_ = new uiCheckBox( amplgrp, tr("Zeros") );
    inc0sbox_->attach( alignedBelow, clipfld_ );
    inc0sbox_->setHSzPol( uiObject::Small );
    inc0sbox_->setToolTip( tr("Include value '0' for histogram display") );
    inc0sbox_->activated.notify( adupcb );

    return amplgrp;
}


uiSEGYReadStarter::~uiSEGYReadStarter()
{
    delete classicsip_;
    delete survinfo_;
    delete timer_;
    delete filereadopts_;
    delete scaninfos_;
    delete &clipsampler_;
}


bool uiSEGYReadStarter::fileIsInTime() const
{
    return SI().zIsTime() != zdombox_->isChecked();
}


void uiSEGYReadStarter::setZIsTime( bool yn )
{
    infofld_->setZIsTime( yn );
}


FullSpec uiSEGYReadStarter::fullSpec() const
{
    const SEGY::ImpType& imptyp = impType();
    FullSpec ret( imptyp.geomType(), imptyp.isVSP() );
    ret.rev_ = loaddef_.revision_;
    ret.spec_ = filespec_;
    ret.pars_ = filepars_;
    ret.zinfeet_ = zInFeet();
    if ( filereadopts_ )
	ret.readopts_ = *filereadopts_;
    return ret;
}


void uiSEGYReadStarter::clearDisplay()
{
    infofld_->clearInfo();
    if ( ampldisp_ )
	ampldisp_->setEmpty();
    if ( mForSurvSetup )
	survmap_->setSurveyInfo( 0 );
    setToolStates();
}


void uiSEGYReadStarter::setImpTypIdx( int tidx )
{
    if ( imptypeFixed() )
	{ pErrMsg( "Cannot set type if fixed" ); return; }

    typfld_->setTypIdx( tidx ); // should trigger its callback
}


const SEGY::ImpType& uiSEGYReadStarter::impType() const
{
    return typfld_ ? typfld_->impType() : fixedimptype_;
}


void uiSEGYReadStarter::execNewScan( LoadDefChgType ct, bool full )
{
    delete scaninfos_; scaninfos_ = 0;
    clipsampler_.reset();
    clearDisplay();
    if ( !getFileSpec() )
	return;

    const SEGY::ImpType& imptyp = impType();
    scaninfos_ = new SEGY::ScanInfoSet( imptyp.is2D(), imptyp.isPS() );
    scaninfos_->setName( userfilename_ );

    PtrMan<TaskRunner> trunner;
    if ( full )
	trunner = new uiTaskRunner( this );

    uiUserShowWait usw( this, uiStrings::sCollectingData() );
    const BufferString fnm0 = filespec_.fileName( 0 );
    if ( !scanFile(fnm0,ct,trunner) )
	return;

    if ( !full && detectrev0flds_ )
    {
	detectrev0flds_ = false;
	scaninfos_->scanInfo(0).keyData().setBest( *loaddef_.hdrdef_ );
	scaninfos_->setEmpty();
	if ( !scanFile(fnm0,ct,trunner) )
	    return;
    }

    const int nrfiles = filespec_.nrFiles();
    for ( int idx=1; idx<nrfiles; idx++ )
    {
	if ( !scanFile(filespec_.fileName(idx),KeepAll,trunner) )
	    break;
    }

    scaninfos_->finish();
    displayScanResults();
    lastscanwasfull_ = full;
}


bool uiSEGYReadStarter::needICvsXY() const
{
    const SEGY::ImpType imptyp = impType();
    return !imptyp.is2D() && !imptyp.isVSP();
}


int uiSEGYReadStarter::examineNrTraces() const
{
    return examinenrtrcsfld_->getIntValue();
}


float uiSEGYReadStarter::ratioClip() const
{
    float clipperc = clipfld_->getFValue();
    const bool useclip = !mIsUdf(clipperc) && clipperc > 0.05;
    return useclip ? clipperc * 0.01f : 0.f;
}


bool uiSEGYReadStarter::incZeros() const
{
    return inc0sbox_->isChecked();
}


void uiSEGYReadStarter::setToolStates()
{
    const int nrfiles = filespec_.nrFiles();
    const bool haveany = nrfiles > 0;
    const bool ismulti = nrfiles > 1;
    examinebut_->setSensitive( haveany );
    if ( fullscanbut_ )
	fullscanbut_->setSensitive( haveany );

    examinebut_->setToolTip( ismulti ? tr("Examine first input file")
				     : tr("Examine input file") );
    const SEGY::ImpType imptyp = impType();
    if ( hdrentrysettsbut_ )
    {
	bool dodisp = !imptyp.isVSP();
	if ( !imptyp.is2D() && !imptyp.isPS() && loaddef_.revision_ > 0 )
	    dodisp= false;
	hdrentrysettsbut_->display( dodisp );
    }
    if ( keepzsampbox_ )
    {
	keepzsampbox_->display( imptyp.is2D() && ismulti );
	keepZChg( 0 );
    }
    if ( useicbut_ )
    {
	const bool isneeded = needICvsXY();
	useicbut_->display( isneeded );
	usexybut_->display( isneeded );
	if ( isneeded )
	    updateICvsXYButtons();
    }

    if ( coordscalefld_ )
	coordscalefld_->display( loaddef_.needXY() );

    const bool shoulddisplay = SI().getCoordSystem() &&
		SI().getCoordSystem()->isProjection() &&
		usexybut_ && usexybut_->isChecked();
    coordsysselfld_->display( shoulddisplay );

    if ( editbut_ )
	editbut_->setSensitive( nrfiles==1 &&
				File::exists(filespec_.fileName(0)) );
}


void uiSEGYReadStarter::initWin( CallBacker* )
{
    typChg( 0 );
    if ( !mForSurvSetup )
	inpChg( 0 );

    if ( filespec_.isEmpty() )
    {
	timer_ = new Timer( "uiSEGYReadStarter timer" );
	timer_->tick.notify( mCB(this,uiSEGYReadStarter,firstSel));
	timer_->start( 1, true );
    }

    if ( (imptypeFixed() && fixedimptype_.isVSP()) )
	return;

    if ( !setup_.vintagecheckmode_ )
    {
	uiButton* okbut = button( OK );
	const CallBack impcb( mCB(this,uiSEGYReadStarter,runClassicImp) );
	const CallBack linkcb( mCB(this,uiSEGYReadStarter,runClassicLink) );
	uiPushButton* execoldbut = new uiPushButton( okbut->parent(),
					    tr("'Classic'"), impcb, false );
	execoldbut->setIcon( "launch" );
	execoldbut->setToolTip( tr("Run the classic SEG-Y loader") );
	execoldbut->attach( leftTo, okbut );
	execoldbut->attach( leftBorder );

	if ( !mForSurvSetup )
	{
	    uiMenu* mnu = execoldbut->addMenu();
	    mnu->insertAction( new uiAction(uiStrings::sImport(),impcb) );
	    mnu->insertAction( new uiAction(uiStrings::sLink(),linkcb) );
	}
    }
}


void uiSEGYReadStarter::firstSel( CallBacker* )
{
    if ( timer_ )
	timer_->tick.remove( mCB(this,uiSEGYReadStarter,firstSel));

    if ( inpfld_ )
    {
	uiFileSelector::Setup fssu;
	fssu.formats( uiSEGYFileSpec::fileFmts() );
	if ( mForSurvSetup )
	    fssu.initialselectiondir( GetBaseDataDir() );
	else
	    fssu.initialselectiondir( sImportFromPath );
	uiFileSelector uifs( this, fssu );
	if ( setup_.vintagecheckmode_ )
	{
	    uiString& capstr = uifs.caption();
	    capstr.set( "Select example file for vintage" );
	}

	if ( !uifs.go() )
	    done( Rejected );
	else
	    inpfld_->setFileName( uifs.fileName() );
    }

    forceRescan( KeepNone );
}


void uiSEGYReadStarter::typChg( CallBacker* )
{
    const SEGY::ImpType& imptyp = impType();

    loaddef_.is2d_ = imptyp.is2D();
    if ( imptyp.is2D() )
    {
	loaddef_.icvsxytype_ = SEGY::FileReadOpts::XYOnly;
	loaddef_.havetrcnrs_ = true;
    }

    infofld_->setImpTypIdx( imptyp.tidx_, false );
    detectrev0flds_ = true;
    forceRescan( KeepBasic );
    setToolStates();
    defaultpar_.setEmpty();
    fillPar( defaultpar_ );
}


void uiSEGYReadStarter::zDomChgCB( CallBacker* )
{
    infofld_->setZIsTime( fileIsInTime() );
}


void uiSEGYReadStarter::coordSysChangedCB( CallBacker* )
{
    if ( *coordsysselfld_->getCoordSystem() == *SI().getCoordSystem()
	    || userfilename_.isEmpty() )
	return;

    execNewScan( KeepNone, false );
    setToolStates();
}


void uiSEGYReadStarter::inpChg( CallBacker* )
{
    detectrev0flds_ = true;
    handleNewInputSpec( KeepNone );
    setToolStates();
    defaultpar_.setEmpty();
    fillPar( defaultpar_ );
}


void uiSEGYReadStarter::revChg( CallBacker* )
{
    detectrev0flds_ = true;
    forceRescan( KeepBasic );
}


void uiSEGYReadStarter::fullScanReq( CallBacker* cb )
{
    forceRescan( KeepAll, true );
}


void uiSEGYReadStarter::getSIPInfo( bool& xyinft, int& ztyp, bool& ztypknown,
				    IOPar& imppars, BufferString& usrfnm ) const
{
    xyinft = zInFeet();
    ztyp = (int)uiSurvInfoProvider::getTDInfo( fileIsInTime(), zInFeet() );
    ztypknown = true;
    const SEGY::FullSpec fullspec( fullSpec() );
    fullspec.fillPar( imppars );
    usrfnm = userFileName();

    if ( classicsip_ )
    {
	xyinft = classicsip_->xyInFeet();
	int cztyp = (int)classicsip_->tdInfo( ztypknown );
	if ( ztypknown )
	    ztyp = cztyp;
	ztypknown = true;
	imppars = classicsip_->imppars_;
	usrfnm = classicsip_->userfilename_;
    }
}


void uiSEGYReadStarter::runClassic( bool imp )
{
    if ( mForSurvSetup )
    {
	if ( !survinfo_ )
	    survinfo_ = new SurveyInfo;
	const uiSurvInfoProvider::TDInfo ztyp
		= uiSurvInfoProvider::getTDInfo( fileIsInTime(), zInFeet() );
	classicsip_ = new uiSEGYClassicSurvInfoProvider;
	if ( !classicsip_->runDialog(this,ztyp,*survinfo_,zInFeet()) )
	    { delete classicsip_; classicsip_ = 0; }
	else
	{
	    survinfook_ = true;
	    done( Accepted );
	}
    }
    else
    {
	const Seis::GeomType gt = impType().geomType();
	uiSEGYRead::Setup su( imp ? uiSEGYRead::Import : uiSEGYRead::DirectDef);
	if ( imptypeFixed() )
	    { su.geoms_.erase(); su.geoms_ += gt; }
	else if ( !imp )
	    su.geoms_ -= Seis::Line;

	commit( true );
	const FullSpec fullspec = fullSpec();
	IOPar iop; fullspec.fillPar( iop );
	classicrdr_ = new uiSEGYRead( this, su, &iop );
	if ( !timer_ )
	    timer_ = new Timer( "uiSEGYReadStarter Classic pop up timer" );
	timer_->tick.notify( mCB(this,uiSEGYReadStarter,initClassic));
	timer_->start( 1, true );
    }
}


void uiSEGYReadStarter::editFile( CallBacker* )
{
    const BufferString fnm( filespec_.fileName(0) );
    if ( !File::exists(fnm) )
	return;

    uiSEGYFileManip dlg( this, fnm );
    if ( dlg.go() )
    {
	if ( inpfld_ )
	    inpfld_->setFileName( dlg.fileName() );

	forceRescan( KeepNone );
    }
}


void uiSEGYReadStarter::forceRescan( LoadDefChgType ct, bool fullscan )
{
    if ( inpfld_ )
	userfilename_.setEmpty();

    handleNewInputSpec( ct, fullscan );
}


void uiSEGYReadStarter::handleNewInputSpec( LoadDefChgType ct, bool fullscan )
{
    if ( !inpfld_ )
	execNewScan( ct, fullscan );
    else
    {
	const BufferString newusrfnm( inpfld_->fileName() );
	if ( newusrfnm.isEmpty() )
	    { clearDisplay(); return; }

	if ( fullscan || newusrfnm != userfilename_ )
	{
	    userfilename_ = newusrfnm;
	    execNewScan( ct, fullscan );
	}

	File::Path fp( newusrfnm );
	sImportFromPath = fp.pathOnly();
    }

    zDomChgCB( 0 );
}


void uiSEGYReadStarter::examineCB( CallBacker* )
{
    if ( !commit() )
	return;

    uiUserShowWait usw( this, uiStrings::sCollectingData() );
    uiSEGYExamine::Setup su( examineNrTraces() );
    su.fs_ = filespec_; su.fp_ = filepars_;
    uiSEGYExamine* dlg = new uiSEGYExamine( this, su );
    dlg->setDeleteOnClose( true );
    dlg->go();
}


void uiSEGYReadStarter::usePar( const IOPar& iop )
{
    if ( typfld_ )
	typfld_->usePar( iop );

    if ( coordsysselfld_->isDisplayed() )
	coordsysselfld_->getCoordSystem()->usePar( iop );

    int nrtrcs = examineNrTraces();
    iop.get( uiSEGYExamine::Setup::sKeyNrTrcs, nrtrcs );
    examinenrtrcsfld_->setValue( nrtrcs );

    float clipratio = ratioClip();
    iop.get( sKeyClipRatio, clipratio );
    if ( mIsUdf(clipratio) || clipratio < 0.f )
	clipratio = 0.f;
    else if ( clipratio > 1.f )
	clipratio = 1.f;
    clipfld_->setValue( clipratio*100.f );

    bool inczeros = incZeros();
    iop.getYN( sKeyIncludeZeros, inczeros );
    inc0sbox_->setChecked( inczeros );

    loaddef_.usePar( iop );
    forceRescan( KeepAll, false );
    infofld_->useLoadDef();
    updateICvsXYButtons();
}


void uiSEGYReadStarter::fillPar( IOPar& iop ) const
{
    const_cast<uiSEGYReadStarter*>(this)->commit( true );
    const FullSpec fullspec = fullSpec();
    fullspec.fillPar( iop );

    if ( SI().getCoordSystem().ptr() && SI().getCoordSystem()->isProjection() )
	coordsysselfld_->getCoordSystem()->fillPar( iop );


    iop.set( FilePars::sKeyRevision(), loaddef_.revision_ );
    impType().fillPar( iop );
    iop.set( uiSEGYExamine::Setup::sKeyNrTrcs, examineNrTraces() );
    iop.set( sKeyClipRatio, ratioClip() );
    iop.setYN( sKeyIncludeZeros, incZeros() );
}


void uiSEGYReadStarter::vntRefreshCB( CallBacker* )
{
    usePar( defaultpar_ );
}


void uiSEGYReadStarter::readParsCB( CallBacker* )
{
    uiSEGYReadImpParsDlg dlg( this, lastparname_ );
    if ( dlg.go() )
    {
	usePar( *dlg.pars() );
	lastparname_ = dlg.parName();
    }
}


void uiSEGYReadStarter::writeParsCB( CallBacker* )
{
    writePars();
}


bool uiSEGYReadStarter::writePars()
{
    IOPar iop; fillPar( iop );
    uiSEGYStoreImpParsDlg dlg( this, iop, lastparname_,
			       setup_.vintagecheckmode_ );
    if ( setup_.vintagecheckmode_ )
	dlg.setCaption( tr("Please specify name for this vintage") );

    if ( !dlg.go() )
	return false;

    lastparname_ = dlg.parName();
    return true;
}


void uiSEGYReadStarter::keepZChg( CallBacker* cb )
{
    loaddef_.usezsamplinginfile_ = keepzsampbox_ && keepzsampbox_->isChecked();
    infofld_->showZSamplingSetting( !loaddef_.usezsamplinginfile_
				    || filespec_.nrFiles() < 2 );

    if ( cb )
	forceRescan();
}


void uiSEGYReadStarter::icxyCB( CallBacker* cb )
{
    if ( !useicbut_ ) return;

    const bool iccl = cb == useicbut_;
    const bool useic = iccl ? useicbut_->isChecked() : !usexybut_->isChecked();
    loaddef_.icvsxytype_ = useic ? SEGY::FileReadOpts::ICOnly
				 : SEGY::FileReadOpts::XYOnly;
    if ( iccl )
    {
	NotifyStopper ns( usexybut_->activated );
	usexybut_->setChecked( !useic );
    }
    else
    {
	NotifyStopper ns( useicbut_->activated );
	useicbut_->setChecked( !useic );
    }

    forceRescan();
}


void uiSEGYReadStarter::updateCoordScale()
{
    loaddef_.coordscale_ = mUdf(float);
    if ( !coordscalefld_ )
	return;

    BufferString edtxt = coordscalefld_->text();
    edtxt.trimBlanks();
    if ( edtxt.isEmpty() )
	return;

    loaddef_.coordscale_ = edtxt.toFloat();
}


void uiSEGYReadStarter::coordscaleChg( CallBacker* cb )
{
    updateCoordScale();
    forceRescan();
}


class uiSEGYHdrEntrySettings : public uiDialog
{ mODTextTranslationClass(uiSEGYHdrEntrySettings)
public:

uiSEGYHdrEntrySettings( uiParent* p )
    : uiDialog(p,Setup(tr("SEG-Y byte location scanning settings"),
			mNoDlgTitle,mTODOHelpKey).savebutton(true))
{
    const SEGY::HdrEntryConstraints& hec = SEGY::HdrEntryConstraints::get();
    inlrgfld_ = new uiGenInput( this, tr("Usable Inline Number range"),
				IntInpIntervalSpec(hec.inlrg_) );
    crlrgfld_ = new uiGenInput( this, tr("Usable Crossline Number range"),
				IntInpIntervalSpec(hec.crlrg_) );
    crlrgfld_->attach( alignedBelow, inlrgfld_ );
    trcnrrgfld_ = new uiGenInput( this, tr("Usable 2D Trace Number range"),
				IntInpIntervalSpec(hec.trcnrrg_) );
    trcnrrgfld_->attach( alignedBelow, crlrgfld_ );
    xrgfld_ = new uiGenInput( this, tr("Usable X-coordinate value range"),
				DoubleInpIntervalSpec(hec.xrg_) );
    xrgfld_->attach( alignedBelow, trcnrrgfld_ );
    yrgfld_ = new uiGenInput( this, tr("Usable Y-coordinate value range"),
				DoubleInpIntervalSpec(hec.yrg_) );
    yrgfld_->attach( alignedBelow, xrgfld_ );
    offsrgfld_ = new uiGenInput( this, tr("Usable Offset value range"),
				FloatInpIntervalSpec(hec.offsrg_) );
    offsrgfld_->attach( alignedBelow, yrgfld_ );
    azimuthrgfld_ = new uiGenInput( this, tr("Usable Azimuth value range"),
				    FloatInpIntervalSpec(hec.azimuthrg_) );
    azimuthrgfld_->attach( alignedBelow, offsrgfld_ );
}

bool acceptOK()
{
    SEGY::HdrEntryConstraints& hec = SEGY::HdrEntryConstraints::get4Edit();
    hec.inlrg_ = inlrgfld_->getIInterval();
    hec.crlrg_ = crlrgfld_->getIInterval();
    hec.trcnrrg_ = trcnrrgfld_->getIInterval();
    hec.xrg_ = xrgfld_->getDInterval();
    hec.yrg_ = yrgfld_->getDInterval();
    hec.offsrg_ = offsrgfld_->getFInterval();
    hec.azimuthrg_ = azimuthrgfld_->getFInterval();
    if ( saveButtonChecked() )
	hec.save2Settings();
    return true;
}

    uiGenInput*		inlrgfld_;
    uiGenInput*		crlrgfld_;
    uiGenInput*		trcnrrgfld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		yrgfld_;
    uiGenInput*		offsrgfld_;
    uiGenInput*		azimuthrgfld_;

};


void uiSEGYReadStarter::editHdrEntrySettings( CallBacker* cb )
{
    uiSEGYHdrEntrySettings dlg( this );
    dlg.go();
}


void uiSEGYReadStarter::updateICvsXYButtons()
{
    if ( !useicbut_ ) return;

    const bool useic = loaddef_.icvsxytype_ != SEGY::FileReadOpts::XYOnly;
    NotifyStopper nsic( useicbut_->activated );
    useicbut_->setChecked( useic );
    NotifyStopper nsxy( usexybut_->activated );
    usexybut_->setChecked( !useic );
}


void uiSEGYReadStarter::updateAmplDisplay( CallBacker* )
{
    if ( !ampldisp_ )
	return;

    int nrvals = (int)clipsampler_.nrVals();
    if ( nrvals < 1 )
	{ ampldisp_->setEmpty(); return; }

    const float* csvals = clipsampler_.vals();
    float clipratio = ratioClip();
    const bool useclip = clipratio > 0.0005;
    const bool rm0 = !incZeros();
    if ( !useclip && !rm0 )
	{ ampldisp_->setData( csvals, nrvals ); return; }

    TypeSet<float> vals;
    if ( !rm0 )
	vals.append( csvals, nrvals );
    else
    {
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    const float val = csvals[idx];
	    if ( val != 0.f )
		vals += val;
	}
	nrvals = vals.size();
    }

    if ( nrvals < 1 )
	{ ampldisp_->setEmpty(); return; }

    if ( useclip )
    {
	DataClipper clipper;
	clipper.putData( vals.arr(), nrvals );
	Interval<float> rg;
	clipper.calculateRange( clipratio, rg );
	TypeSet<float> oldvals( vals );
	vals.setEmpty();
	for ( int idx=0; idx<nrvals; idx++ )
	{
	    const float val = oldvals[idx];
	    if ( rg.includes(val,false) )
		vals += val;
	}

	nrvals = vals.size();
	if ( nrvals < 1 )
	    { ampldisp_->setEmpty(); return; }
    }

    ampldisp_->setData( vals.arr(), nrvals );
}


void uiSEGYReadStarter::updateSurvMap()
{
    survinfook_ = false;
    if ( !survmap_ )
	return;

    delete survinfo_;
    survinfo_ = new SurveyInfo;
    survinfo_->setName( "No valid scan available" );
    uiString stbarmsg = uiString::empty();
    if ( scaninfos_ && !scaninfos_->isEmpty() )
    {
	Coord crd[3]; TrcKeyZSampling cs;
	stbarmsg = scaninfos_->piDetector().getSurvInfo( cs.hsamp_, crd );
	if ( stbarmsg.isEmpty() )
	{
	    cs.zsamp_ = loaddef_.getZRange();
	    survinfo_->setRanges( cs );
	    BinID bid[2];
	    bid[0].inl() = cs.hsamp_.start_.inl();
	    bid[0].crl() = cs.hsamp_.start_.crl();
	    bid[1].inl() = cs.hsamp_.stop_.inl();
	    bid[1].crl() = cs.hsamp_.stop_.crl();
	    stbarmsg = survinfo_->set3Pts(crd, bid, cs.hsamp_.stop_.crl());
	}
	if ( stbarmsg.isEmpty() )
	{
	    survinfook_ = true;
	    survinfo_->setName( "Resulting survey setup" );
	}
	else
	    survinfo_->setName( "<Inadequate data>" );
    }

    toStatusBar( stbarmsg );
    survmap_->setSurveyInfo( survinfo_ );
}


bool uiSEGYReadStarter::getInfo4SI( TrcKeyZSampling& cs, Coord crd[3] ) const
{
    if ( !survinfook_ )
	return false;

    BinID bids[2]; int xline;
    survinfo_->get3Pts( crd, bids, xline );
    survinfo_->getSampling( cs );
    return true;
}


void uiSEGYReadStarter::initClassic( CallBacker* )
{
    if ( !classicrdr_ )
	return;

    if ( mForSurvSetup )
	{ mAttachCB( classicrdr_->processEnded,
		     uiSEGYReadStarter::classicSurvSetupEnd ); }

    classicrdr_->raiseCurrent();
}


void uiSEGYReadStarter::classicSurvSetupEnd( CallBacker* )
{
    classicrdr_ = 0;
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiSEGYReadStarter::getFileSpec()
{
    if ( userfilename_.isEmpty() )
	return false;

    filespec_.setEmpty();
    if ( !userfilename_.find('*') )
    {
	if ( !getExistingFileName(userfilename_) )
	    return false;
	filespec_.setFileName( userfilename_ );
    }
    else
    {
	File::Path fp( userfilename_ );
	if ( !fp.isAbsolute() )
	    mErrRet(uiStrings::phrSpecify(tr(
			    "the absolute file name when using a wildcard.")) )

	DirList dl( fp.pathOnly(), File::FilesInDir, fp.fileName() );
	for ( int idx=0; idx<dl.size(); idx++ )
	    filespec_.fnames_.add( dl.fullPath(idx) );

	if ( filespec_.isEmpty() )
	    mErrRet( tr("No file names matching your wildcard(s).") )

	filespec_.setUsrStr( userfilename_ );
    }

    return true;
}


bool uiSEGYReadStarter::getExistingFileName( BufferString& fnm, bool emiterr )
{
    File::Path fp( fnm );
    if ( fp.isAbsolute() )
    {
	if ( !File::exists(fnm) )
	{
	    if ( emiterr )
		uiMSG().error( tr(
			    "SEG-Y file does not exist:\n%1").arg(fnm) );
	    return false;
	}
    }
    else
    {
	File::Path newfp( GetDataDir(), fnm );
	if ( !File::exists(newfp.fullPath()) )
	{
	    newfp.set( GetDataDir() ).add( sSeismicSubDir() );
	    if ( !File::exists(newfp.fullPath()) )
	    {
		if ( emiterr )
		    uiMSG().error(
			    tr("SEG-Y file not found in survey directory") );
		return false;
	    }
	}
	fnm = newfp.fullPath();
    }

    return true;
}


bool uiSEGYReadStarter::scanFile( const char* fnm, LoadDefChgType ct,
				  TaskRunner* tskr )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
	{ uiMSG().error( tr("Cannot open file: %1").arg(fnm) ); return false; }

    SEGY::ScanInfo& si = scaninfos_->add( fnm );
    if ( coordsysselfld_->isDisplayed() )
	loaddef_.setUserCoordSys( coordsysselfld_->getCoordSystem() );
    SEGY::BasicFileInfo& bfi = si.basicInfo();
    bool zinft = false;
    uiString errmsg = bfi.getFrom( strm, zinft,
			       ct != KeepNone ? &loaddef_.hdrsswapped_ : 0 );
    if ( !errmsg.isEmpty() )
	{ uiMSG().error( errmsg ); scaninfos_->removeLast(); return false; }
    else if ( scaninfos_->size() == 1 )
    {
	scaninfos_->setInFeet( zinft );
	if ( ct == KeepNone )
	    static_cast<SEGY::BasicFileInfo&>(loaddef_) = bfi;
    }

    si.getFromSEGYBody( strm, loaddef_, mForSurvSetup, clipsampler_, tskr );
    return true;
}


#define mErrRetResetStream(str) { \
    strm.setReadPosition( firsttrcpos ); \
    uiMSG().error( str.arg(strm.fileName()) ); \
    return false; }

bool uiSEGYReadStarter::completeFileInfo( od_istream& strm,
				      SEGY::BasicFileInfo& bfi, bool isfirst )
{
    const od_stream::Pos firsttrcpos = strm.position();

    PtrMan<SEGY::TrcHeader> thdr = loaddef_.getTrcHdr( strm );
    if ( !thdr )
	mErrRetResetStream( tr("File:\n%1\nNo traces found") )

    if ( bfi.ns_ < 1 )
	bfi.ns_ = (int)thdr->nrSamples();
    if ( bfi.ns_ > SEGY::cMaxReasonableNrSamples() )
	mErrRetResetStream(
	    tr("File:\n%1\nNo proper 'number of samples per trace' found") )

    if ( mIsUdf(bfi.sampling_.step) )
    {
	SeisTrcInfo ti; thdr->fill( ti, loaddef_.is2d_, 1.0f );
	bfi.sampling_ = ti.sampling_;
    }

    strm.setReadPosition( firsttrcpos );
    return true;
}


void uiSEGYReadStarter::displayScanResults()
{
    if ( !scaninfos_ || scaninfos_->isEmpty() )
	{ clearDisplay(); return; }

    setToolStates();
    if ( ampldisp_ )
	updateAmplDisplay( 0 );

    infofld_->setScanInfo( *scaninfos_ );
    if ( mForSurvSetup )
	updateSurvMap();
}


bool uiSEGYReadStarter::commit( bool permissive )
{
    if ( !permissive && filespec_.isEmpty() )
    {
	uiMSG().error( uiStrings::phrSelect(uiStrings::sInputFile()) );
	return false;
    }

    loaddef_.getFilePars( filepars_ );
    loaddef_.usezsamplinginfile_ = keepzsampbox_ && keepzsampbox_->isChecked();
    if ( loaddef_.usezsamplinginfile_ )
	filepars_.ns_ = 0;

    delete filereadopts_;
    filereadopts_ = new FileReadOpts( impType().geomType() );
    loaddef_.getFileReadOpts( *filereadopts_ );

    updateCoordScale();
    return true;
}


bool uiSEGYReadStarter::acceptOK()
{
    if ( setup_.vintagecheckmode_ )
	return true;

    if ( !commit(false) )
	return false;

    if ( mForSurvSetup )
    {
	if ( !survinfook_ )
	    mErrRet( tr("No valid survey setup found" ) )
	if ( !lastscanwasfull_ )
	{
	    const uiString msg( tr("We recommend doing a full file scan to "
		"establish the true ranges and steps of the input file."
		"\nIf you are sure the quick scan has got everything right, "
		"you may want to continue without a full scan."
		"\n\nDo you want to continue without a full scan?") );
	    const int res = uiMSG().askGoOnAfter( msg, uiStrings::sCancel(),
			tr("Yes, continue"), tr("No, execute full scan now") );
	    if ( res < 1 )
	    {
		if ( res == 0 )
		    execNewScan( KeepAll, true );
		return false;
	    }
	}
	return true;
    }

    const FullSpec fullspec = fullSpec();
    uiSEGYReadFinisher dlg( this, fullspec, userfilename_, fileIsInTime() );
    dlg.go();

    return false;
}


bool uiSEGYReadStarter::isExampleVntSelected( const BufferString& inpnm )
{
    if ( !setup_.vntinfos_ )
	return false;

    for ( int vntidx=0; vntidx<setup_.vntinfos_->size(); vntidx++ )
    {
	BufferString fnm;
	const SEGY::Vintage::Info* vntinfo = setup_.vntinfos_->get(vntidx);
	Repos::IOParSet parset = Repos::IOParSet( "SEGYSetups" );
	int selidx = parset.find( vntinfo->vintagenm_ );
	Repos::IOPar* iop = selidx<0 ? 0 : parset[selidx];
	if ( iop && inpnm == iop->find(sKey::FileName()) )
	    return true;
    }

    return false;
}
