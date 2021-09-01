
  /*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
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
#include "uisegymultilinesel.h"
#include "uisegydef.h"
#include "uisegyread.h"
#include "uifileinput.h"
#include "uifiledlg.h"
#include "uiseparator.h"
#include "uisurvmap.h"
#include "uihistogramdisplay.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uispinbox.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uimenu.h"
#include "uisplitter.h"

#include "dataclipper.h"
#include "dirlist.h"
#include "envvars.h"
#include "filepath.h"
#include "oddirs.h"
#include "od_istream.h"
#include "posinfodetector.h"
#include "segyhdr.h"
#include "segyhdrkeydata.h"
#include "seisinfo.h"
#include "settings.h"
#include "survinfo.h"
#include "timer.h"


#define mSurvMapHeight 300
#define mDefSize 250
#define mClipSamplerBufSz 100000

static const char* sKeyClipRatio = "Amplitudes.Clip Ratio";
static const char* sKeyIncludeZeros = "Amplitudes.Include Zeros";
static BufferString sImportFromPath;

uiSEGYReadStarter::uiSEGYReadStarter( uiParent* p, bool forsurvsetup,
					const SEGY::ImpType* imptyp )
    : uiDialog(p,uiDialog::Setup(forsurvsetup
	    ? tr("Extract Survey Setup from SEG-Y") : tr("Import SEG-Y Data"),
	    imptyp ? tr("Import %1").arg(imptyp->dispText())
		   : mNoDlgTitle,
	    mODHelpKey(mSEGYReadStarterHelpID)).nrstatusflds(1)
					       .modal(forsurvsetup))
    , forsurvsetup_(forsurvsetup)
    , filereadopts_(nullptr)
    , typfld_(nullptr)
    , multilinebut_(nullptr)
    , useicbut_(nullptr)
    , usexybut_(nullptr)
    , coordscalefld_(nullptr)
    , ampldisp_(nullptr)
    , survmap_(nullptr)
    , detectrev0flds_(true)
    , userfilename_("_") // any non-empty non-existing
    , scaninfos_(nullptr)
    , clipsampler_(*new DataClipSampler(100000))
    , lastscanwasfull_(false)
    , survinfo_(nullptr)
    , survinfook_(false)
    , timer_(nullptr)
{
    if ( forsurvsetup )
	loaddef_.icvsxytype_ = SEGY::FileReadOpts::Both;
    else
    {
	setCtrlStyle( RunAndClose );
	setOkText( tr("Next >>") );
	loaddef_.icvsxytype_ = SEGY::FileReadOpts::ICOnly;
    }

    if ( sImportFromPath.isEmpty() )
	sImportFromPath = GetDataDir();

    topgrp_ = new uiGroup( this, "Top group" );
    uiFileInput::Setup fisu( uiFileDialog::Gen, filespec_.fileName() );
    fisu.filter( uiSEGYFileSpec::fileFilter() ).forread( true )
	.objtype( tr("SEG-Y") ).defseldir(sImportFromPath);
    inpfld_ = new uiFileInput( topgrp_, uiStrings::phrJoinStrings(
			       uiStrings::sInputFile(),tr("*=wildcard")),fisu );
    mAttachCB( inpfld_->valuechanged, uiSEGYReadStarter::inpChg );
    editbut_ = uiButton::getStd( topgrp_, OD::Edit,
				 mCB(this,uiSEGYReadStarter,editFile), false );
    editbut_->attach( rightOf, inpfld_ );
    editbut_->setSensitive( false );

    uiObject* attachobj = inpfld_->attachObj();

    if ( imptyp )
	fixedimptype_ = *imptyp;
    else
    {
	typfld_ = new uiSEGYImpType( topgrp_, !forsurvsetup );
	mAttachCB( typfld_->typeChanged, uiSEGYReadStarter::typChg );
	typfld_->attach( alignedBelow, inpfld_ );
	attachobj = typfld_->attachObj();
    }

    if ( typfld_ || fixedimptype_.is2D() )
    {
	multilinebut_ = new uiPushButton( topgrp_, tr("Review/Edit Line names"),
					  false );
	mAttachCB( multilinebut_->activated, uiSEGYReadStarter::multiLineSelCB);
	multilinebut_->display( false );
	if ( typfld_ )
	    multilinebut_->attach( rightTo, typfld_ );
	else
	{
	    multilinebut_->attach( alignedBelow, inpfld_ );
	    attachobj = multilinebut_;
	}
    }

    coordsysselfld_ = new Coords::uiCoordSystemSel( topgrp_ );
    coordsysselfld_->attach( alignedBelow, attachobj );

    mAttachCB( coordsysselfld_->butPush, uiSEGYReadStarter::coordSysChangedCB );
    coordsysselfld_->display( impType().is2D() );

    uiSeparator* sep = new uiSeparator( this, "Top sep" );
    sep->attach( stretchedBelow, topgrp_ );

    midgrp_ = new uiGroup( this, "Mid group" );
    midgrp_->attach( ensureBelow, sep );
    infofld_ = new uiSEGYReadStartInfo( midgrp_, loaddef_, imptyp );
    mAttachCB( infofld_->loaddefChanged, uiSEGYReadStarter::defChg );
    mAttachCB( infofld_->revChanged, uiSEGYReadStarter::revChg );

    sep = new uiSeparator( this, "Bot sep" );
    sep->attach( stretchedBelow, midgrp_ );

    botgrp_ = new uiGroup( this, "Bottom group" );
    botgrp_->attach( ensureBelow, sep );
    if ( forsurvsetup )
    {
	survmap_ = new uiSurveyMap( botgrp_, true );
	survmap_->setSurveyInfo( 0 );
    }

    uiGroup* amplgrp = createAmplDisp();
    if ( survmap_ )
    {
	survmap_->view().setPrefWidth( mDefSize );
	amplgrp->attach( rightTo, survmap_ );
    }

    createTools();
    botgrp_->setStretch( 2, 1 );

    setToolStates();
    mAttachCB( postFinalise(), uiSEGYReadStarter::initWin );
}


void uiSEGYReadStarter::createTools()
{
    uiGroup* toolgrp = new uiGroup( midgrp_, "Tool group" );
    toolgrp->setStretch( 0, 0 );
    toolgrp->attach( rightOf, infofld_ );
    uiToolButton* openbut = new uiToolButton( toolgrp, "open",
				tr("Use Saved SEG-Y setup"),
				mCB(this,uiSEGYReadStarter,readParsCB) );
    uiToolButton* savebut = new uiToolButton( toolgrp, "save",
				tr("Store this setup"),
				mCB(this,uiSEGYReadStarter,writeParsCB) );
    savebut->attach( rightOf, openbut );

    fullscanbut_ = new uiToolButton( toolgrp, "fullscan",
				    tr("Scan the entire input"),
				    mCB(this,uiSEGYReadStarter,fullScanReq) );
    fullscanbut_->attach( alignedBelow, openbut );
    hdrentrysettsbut_ = new uiToolButton( toolgrp, "settings",
			    tr("Settings for byte location scanning"),
			    mCB(this,uiSEGYReadStarter,editHdrEntrySettings) );
    hdrentrysettsbut_->attach( rightOf, fullscanbut_ );

    uiGroup* examinegrp = new uiGroup( toolgrp, "Examine group" );
    examinebut_ = new uiToolButton( examinegrp, "examine",
				    uiString::emptyString(),
				    mCB(this,uiSEGYReadStarter,examineCB) );
    examinenrtrcsfld_ = new uiSpinBox( examinegrp, 0, "Examine traces" );
    examinenrtrcsfld_->setInterval( 0, 1000000, 10 );
    examinenrtrcsfld_->setHSzPol( uiObject::Small );
    examinenrtrcsfld_->setToolTip( tr("Number of traces to examine") );
    examinenrtrcsfld_->attach( alignedBelow, examinebut_ );
    int nrex = 1000; Settings::common().get( sKeySettNrTrcExamine, nrex );
    examinenrtrcsfld_->setInterval( 10, 1000000, 10 );
    examinenrtrcsfld_->setValue( nrex );
    examinegrp->attach( alignedBelow, fullscanbut_ );
    examinegrp->setFrame( true );

    if ( imptypeFixed() && fixedimptype_.isVSP() )
	return;

    uiObject* lowest = examinegrp->attachObj();

    const bool imptypfixed = imptypeFixed();
    bool needicvsxy = !forsurvsetup_;
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
	    tr( "Select to use (X,Y) for positioning.\nInline/Crossline "
		"will be calculated using the survey setup.\nOnly use this "
		"option if you do not have valid In- and Crossline numbers.") );
	usexybut_->attach( alignedBelow, useicbut_ );
	lowest = usexybut_;
    }

    coordscalefld_ = new uiLineEdit( toolgrp, FloatInpSpec(),"CoordScale");
    coordscalefld_->setHSzPol( uiObject::Small );
    coordscalefld_->setToolTip( tr( "Enter a value if you want to ignore "
	"the coordinate scaling in the trace headers."
	"\nAll coordinates read will then be multiplied by that factor."
	"\nEntering 0 will reset the scaling back to scale found in header "
	"(scalco byte 71-72)." ));
    coordscalefld_->setPlaceholderText( tr("scale XY") );
    mAttachCB(coordscalefld_->returnPressed, uiSEGYReadStarter::coordscaleChg );
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
    ampldisp_->setTitle( tr("Amplitudes") );

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
    detachAllNotifiers();
    delete survinfo_;
    delete timer_;
    delete filereadopts_;
    delete scaninfos_;
    delete &clipsampler_;
}


FullSpec uiSEGYReadStarter::fullSpec() const
{
    const SEGY::ImpType& imptyp = impType();
    FullSpec ret( imptyp.geomType(), imptyp.isVSP() );
    ret.rev_ = loaddef_.revision_;
    ret.spec_ = filespec_;
    ret.linenames_ = linenames_;
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
    if ( forsurvsetup_ )
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


void uiSEGYReadStarter::multiLineSelCB( CallBacker* )
{ reviewAndEditLineNames(); }

bool uiSEGYReadStarter::reviewAndEditLineNames()
{
    uiSEGYMultiLineSel linedlg( this, filespec_, wcidx_, linenames_ );
    return linedlg.go();
}


void uiSEGYReadStarter::execNewScan( LoadDefChgType ct, bool full )
{
    delete scaninfos_; scaninfos_ = 0;
    clipsampler_.reset();
    clearDisplay();
    if ( !getFileSpec() )
	return;

    if ( multilinebut_ )
	multilinebut_->display( impType().is2D() && filespec_.nrFiles() > 1 );

    const SEGY::ImpType& imptyp = impType();
    scaninfos_ = new SEGY::ScanInfoSet( imptyp.is2D(), imptyp.isPS() );
    scaninfos_->setName( userfilename_ );

    PtrMan<TaskRunner> trunner;
    if ( full )
	trunner = new uiTaskRunner( this );

    MouseCursorChanger chgr( MouseCursor::Wait );
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

    if ( ct == uiSEGYReadStarter::KeepNone )
    {
	const bool isrev0 = loaddef_.revision_==0;
	if ( !isrev0 )
	{
	    infofld_->setRev1Values();
	    infofld_->fillLoadDef();
	}
    }
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

    bool coordisprojection = SI().getCoordSystem() &&
		SI().getCoordSystem()->isProjection();

    if ( impType().is2D() )
    {
	const bool shoulddisplay = coordisprojection;
	coordsysselfld_->display( shoulddisplay );
    }
    else
    {
	const bool isxybutused = usexybut_ && usexybut_->isChecked();
	const bool shoulddisplay = coordisprojection && isxybutused;
	coordsysselfld_->display( shoulddisplay );
    }

    editbut_->setSensitive( nrfiles==1 && File::exists(filespec_.fileName(0)) );
}


void uiSEGYReadStarter::initWin( CallBacker* )
{
    typChg( 0 );
    if ( !forsurvsetup_ )
	inpChg( 0 );

    if ( filespec_.isEmpty() )
    {
	timer_ = new Timer( "uiSEGYReadStarter timer" );
	timer_->tick.notify( mCB(this,uiSEGYReadStarter,firstSel));
	timer_->start( 1, true );
    }

    if ( forsurvsetup_ || (imptypeFixed() && fixedimptype_.isVSP()) )
	return;

    const bool showclassic = GetEnvVarYN( "OD_SEGY_IMPORT_CLASSIC", false );
    if ( showclassic )
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
	uiMenu* mnu = new uiMenu;
	mnu->insertAction( new uiAction(uiStrings::sImport(),impcb) );
	mnu->insertAction( new uiAction(tr("Link"),linkcb) );
	execoldbut->setMenu( mnu );
    }
}


void uiSEGYReadStarter::firstSel( CallBacker* )
{
    timer_->tick.remove( mCB(this,uiSEGYReadStarter,firstSel));

    uiFileDialog dlg( this, uiFileDialog::ExistingFile, 0,
	    uiSEGYFileSpec::fileFilter(),
	    tr("Select (one of) the SEG-Y file(s)") );
    if ( forsurvsetup_ )
	dlg.setDirectory( GetBaseDataDir() );
    else
	dlg.setDirectory( sImportFromPath );

    if ( dlg.go() )
    {
	inpfld_->setFileName( dlg.fileName() );
	forceRescan( KeepNone );
    }
}


void uiSEGYReadStarter::typChg( CallBacker* )
{
    const SEGY::ImpType& imptyp = impType();

    if ( imptyp.is2D() )
	loaddef_.icvsxytype_ = SEGY::FileReadOpts::XYOnly;

    if ( multilinebut_ )
	multilinebut_->display( imptyp.is2D() && filespec_.nrFiles() > 1 );

    infofld_->setImpTypIdx( imptyp.tidx_, false );
    detectrev0flds_ = true;
    forceRescan( KeepBasic );
    setToolStates();
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
    linenames_.setEmpty();
    handleNewInputSpec( KeepNone );
    setToolStates();
}


void uiSEGYReadStarter::fullScanReq( CallBacker* cb )
{
    forceRescan( KeepAll, true );
}


void uiSEGYReadStarter::revChg( CallBacker* )
{
    detectrev0flds_ = true;
    forceRescan( KeepBasic );
}


void uiSEGYReadStarter::runClassic( bool imp )
{
    const Seis::GeomType gt = impType().geomType();
    uiSEGYRead::Setup su( forsurvsetup_ ? uiSEGYRead::SurvSetup
				 : (imp ? uiSEGYRead::Import
					: uiSEGYRead::DirectDef) );
    if ( imptypeFixed() )
	{ su.geoms_.erase(); su.geoms_ += gt; }
    else if ( !imp )
	su.geoms_ -= Seis::Line;

    commit( true );
    const FullSpec fullspec = fullSpec();
    IOPar iop; fullspec.fillPar( iop );
    classicrdr_ = new uiSEGYRead( this, su, &iop );
    if ( !timer_ )
	timer_ = new Timer( "uiSEGYReadStarter timer" );

    timer_->tick.notify( mCB(this,uiSEGYReadStarter,initClassic));
    timer_->start( 1, true );
}


void uiSEGYReadStarter::editFile( CallBacker* )
{
    const BufferString fnm( filespec_.fileName(0) );
    if ( !File::exists(fnm) )
	return;

    uiSEGYFileManip dlg( this, fnm );
    if ( dlg.go() )
    {
	inpfld_->setFileName( dlg.fileName() );
	forceRescan( KeepNone );
    }
}


void uiSEGYReadStarter::forceRescan( LoadDefChgType ct, bool fullscan )
{
    userfilename_.setEmpty();
    handleNewInputSpec( ct, fullscan );
}


void uiSEGYReadStarter::handleNewInputSpec( LoadDefChgType ct, bool fullscan )
{
    const BufferString newusrfnm( inpfld_->fileName() );
    if ( newusrfnm.isEmpty() )
	{ clearDisplay(); return; }

    if ( fullscan || newusrfnm != userfilename_ )
    {
	userfilename_ = newusrfnm;
	execNewScan( ct, fullscan );
    }

    FilePath fp( newusrfnm );
    sImportFromPath = fp.pathOnly();
}


void uiSEGYReadStarter::examineCB( CallBacker* )
{
    if ( !commit() )
	return;

    MouseCursorChanger chgr( MouseCursor::Wait );
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
    IOPar iop; fillPar( iop );
    uiSEGYStoreImpParsDlg dlg( this, iop, lastparname_ );
    if ( dlg.go() )
	lastparname_ = dlg.parName();
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
    const bool reset = edtxt == "0" || edtxt.isEmpty();
    if ( reset )
	coordscalefld_->setEmpty();

    loaddef_.coordscale_ = reset ? mUdf(float) : edtxt.toFloat();
}


void uiSEGYReadStarter::coordscaleChg( CallBacker* )
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
    inlrgfld_ = new uiGenInput( this, tr("Usable In-line number range"),
				IntInpIntervalSpec(hec.inlrg_) );

    crlrgfld_ = new uiGenInput( this, tr("Usable Cross-line number range"),
				IntInpIntervalSpec(hec.crlrg_) );
    crlrgfld_->attach( alignedBelow, inlrgfld_ );

    trcnrrgfld_ = new uiGenInput( this, tr("Usable 2D Trace number range"),
				IntInpIntervalSpec(hec.trcnrrg_) );
    trcnrrgfld_->attach( alignedBelow, crlrgfld_ );

    spnrrgfld_ = new uiGenInput( this, tr("Usable 2D SP number range"),
				IntInpIntervalSpec(hec.refnrrg_) );
    spnrrgfld_->attach( alignedBelow, trcnrrgfld_ );

    xrgfld_ = new uiGenInput( this, tr("Usable X-coordinate value range"),
				DoubleInpIntervalSpec(hec.xrg_) );
    xrgfld_->attach( alignedBelow, spnrrgfld_ );

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

bool acceptOK( CallBacker* )
{
    SEGY::HdrEntryConstraints& hec = SEGY::HdrEntryConstraints::get4Edit();
    hec.inlrg_ = inlrgfld_->getIInterval();
    hec.crlrg_ = crlrgfld_->getIInterval();
    hec.trcnrrg_ = trcnrrgfld_->getIInterval();
    hec.refnrrg_ = spnrrgfld_->getIInterval();
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
    uiGenInput*		spnrrgfld_;
    uiGenInput*		xrgfld_;
    uiGenInput*		yrgfld_;
    uiGenInput*		offsrgfld_;
    uiGenInput*		azimuthrgfld_;

};


void uiSEGYReadStarter::editHdrEntrySettings( CallBacker* )
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

    od_int64 nrvals = clipsampler_.nrVals();
    if ( nrvals < 1 )
	{ ampldisp_->setEmpty(); return; }

    const float* csvals = clipsampler_.vals();
    const float clipratio = ratioClip();
    const bool useclip = clipratio > 0.0005f;
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

    survinfo_ = survmap_->getEmptySurvInfo();
    survinfo_->setName( "No valid scan available" );
    uiString stbarmsg;
    if ( scaninfos_ && !scaninfos_->isEmpty() )
    {
	Coord crd[3]; TrcKeyZSampling cs;
	stbarmsg = scaninfos_->piDetector().getSurvInfoWithMsg( cs.hsamp_, crd);
	if ( stbarmsg.isEmpty() )
	{
	    cs.zsamp_ = loaddef_.getZRange();
	    survinfo_->setRange( cs, false );
	    BinID bid[2];
	    bid[0].inl() = cs.hsamp_.start_.inl();
	    bid[0].crl() = cs.hsamp_.start_.crl();
	    bid[1].inl() = cs.hsamp_.stop_.inl();
	    bid[1].crl() = cs.hsamp_.stop_.crl();
	    stbarmsg = survinfo_->set3PtsWithMsg( crd, bid,
						  cs.hsamp_.stop_.crl() );
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
    cs = survinfo_->sampling( false );
    return true;
}


void uiSEGYReadStarter::initClassic( CallBacker* )
{
    if ( !classicrdr_ )
	return;

    classicrdr_->raiseCurrent();
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
	FilePath fp( userfilename_ );
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
    FilePath fp( fnm );
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
	FilePath newfp( GetDataDir(), fnm );
	if ( !File::exists(newfp.fullPath()) )
	{
	    newfp.set( GetDataDir() ).add( "Seismics" );
	    if ( !File::exists(newfp.fullPath()) )
	    {
		if ( emiterr )
		    uiMSG().error(
			    tr("SEG-Y file not found in survey.") );
		return false;
	    }
	}
	fnm = newfp.fullPath();
    }

    return true;
}


bool uiSEGYReadStarter::scanFile( const char* fnm, LoadDefChgType ct,
				  TaskRunner* trunner )
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
	{
	    static_cast<SEGY::BasicFileInfo&>(loaddef_) = bfi;
	    infofld_->setLoadDefCache( loaddef_ );
	}
    }

    si.getFromSEGYBody( strm, loaddef_, forsurvsetup_, clipsampler_, trunner );
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
	SeisTrcInfo ti; thdr->fill( ti, 1.0f );
	bfi.sampling_ = ti.sampling;
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
    if ( forsurvsetup_ )
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

    delete filereadopts_;
    filereadopts_ = new FileReadOpts( impType().geomType() );
    loaddef_.getFileReadOpts( *filereadopts_ );

    updateCoordScale();
    return true;
}


bool uiSEGYReadStarter::acceptOK( CallBacker* )
{
    if ( !commit(false) )
	return false;

    if ( forsurvsetup_ )
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
    else if ( impType().is2D() && filespec_.nrFiles() > 1 &&
	      linenames_.isEmpty() && !reviewAndEditLineNames() )
	return false;

    const FullSpec fullspec = fullSpec();
    uiSEGYReadFinisher dlg( this, fullspec, userfilename_ );
    dlg.setCoordSystem( coordsysselfld_->getCoordSystem() );
    dlg.go();

    return false;
}
