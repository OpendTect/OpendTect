/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
________________________________________________________________________

-*/

#include "uiwelltiemgrdlg.h"

#include "elasticpropsel.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seistrctr.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "wavelet.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltransl.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"
#include "welltiegeocalculator.h"
#include "wellodreader.h"
#include "welld2tmodel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiseparator.h"
#include "uisynthseis.h"
#include "uiwaveletextraction.h"
#include "uiwellpropertyrefsel.h"
#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecheckshotedit.h"
#include "od_helpids.h"


namespace WellTie
{

uiTieWinMGRDlg::uiTieWinMGRDlg( uiParent* p, WellTie::Setup& wtsetup )
	: uiDialog(p,uiDialog::Setup(tr("Tie Well To Seismics"),
		tr("Select Data to tie Well to Seismic"),
		mODHelpKey(mWellTiMgrDlemgHelpID) )
		.savebutton(true)
		.savechecked(false)
		.modal(false))
	, wtsetup_(wtsetup)
	, elpropsel_(*new ElasticPropSelection(false))
{
    setVideoKey( mODVideoKey(mWellTiMgrDlemgHelpID) );
    setCtrlStyle( RunAndClose );

    const IOObjContext wellctxt = mIOObjContext(Well);
    wellfld_ = new uiIOObjSel( this, wellctxt );
    if ( !wtsetup_.wellid_.isUdf() )
	wellfld_->setInput( wtsetup_.wellid_ );

    const CallBack wllselcb( mCB(this,uiTieWinMGRDlg,wellSelChg) );
    mAttachCB( wellfld_->selectionDone, uiTieWinMGRDlg::wellSelChg );

    auto* sep = new uiSeparator( this, "Well2Seismic Sep" );
    sep->attach( stretchedBelow, wellfld_ );

    auto* seisgrp = new uiGroup( this, "Seismic selection group" );
    seisgrp->attach( ensureBelow, sep );
    seisgrp->attach( alignedBelow, wellfld_ );

    const bool has2d = SI().has2D();
    const bool has3d = SI().has3D();

    if ( has2d && has3d )
    {
	BufferStringSet seistypes;
	seistypes.add( Seis::nameOf(Seis::Line) );
	seistypes.add( Seis::nameOf(Seis::Vol) );
	typefld_ = new uiGenInput( seisgrp, tr("Seismic"),
				   StringListInpSpec( seistypes ) );
	typefld_->setValue( true );
	mAttachCB( typefld_->valuechanged, uiTieWinMGRDlg::typeSelChg );
    }

    if ( has2d )
    {
	uiSeisSel::Setup seis2dfldsetup = uiSeisSel::Setup(Seis::Line);
	seis2dfldsetup.allowsetsurvdefault_ = true;
	seis2dfldsetup.optional_ = true;
	const IOObjContext seis2dctxt =
			uiSeisSel::ioContext( seis2dfldsetup.geom_, true );
	seis2dfld_ = new uiSeisSel( seisgrp, seis2dctxt, seis2dfldsetup );
	seis2dfld_->setChecked( true );
	seis2dfld_->display( !has3d );
	if ( typefld_ )
	    seis2dfld_->attach( alignedBelow, typefld_ );

	mAttachCB( seis2dfld_->selectionDone, uiTieWinMGRDlg::seisSelChg );
	mAttachCB( seis2dfld_->optionalChecked, uiTieWinMGRDlg::seis2DCheckChg);

	seislinefld_ = new uiSeis2DLineNameSel( seisgrp, true );
	seislinefld_->display( !has3d );
	if ( !seis2dfld_->key().isUdf() )
	    seislinefld_->setDataSet( seis2dfld_->key() );

	seislinefld_->attach( alignedBelow, seis2dfld_ );
    }

    if ( has3d )
    {
	uiSeisSel::Setup seis3dfldsetup = uiSeisSel::Setup(Seis::Vol);
	seis3dfldsetup.allowsetsurvdefault_ = true;
	seis3dfldsetup.optional_ = true;
	const IOObjContext seis3dctxt =
			uiSeisSel::ioContext( seis3dfldsetup.geom_, true );
	seis3dfld_ = new uiSeisSel( seisgrp, seis3dctxt, seis3dfldsetup );
	seis3dfld_->setChecked( true );
	if ( typefld_ )
	    seis3dfld_->attach( alignedBelow, typefld_ );

	mAttachCB( seis3dfld_->selectionDone, uiTieWinMGRDlg::seisSelChg );
    }

    seisgrp->setHAlignObj( typefld_ ? (uiGroup*)typefld_
				    : (uiGroup*)seis2dfld_ ?
				    seis2dfld_ : seis3dfld_ );

    sep = new uiSeparator( this, "Seismic2Log Sep" );
    sep->attach( stretchedBelow, seisgrp );

    auto* logsgrp = new uiGroup( this, "Log selection group" );
    logsgrp->attach( alignedBelow, wellfld_ );
    logsgrp->attach( ensureBelow, sep );

    logsfld_ = new uiWellPropSel( logsgrp, elpropsel_ );
    mAttachCB( logsfld_->logCreated, uiTieWinMGRDlg::wellSelChg );

    used2tmbox_ = new uiCheckBox( logsgrp, tr("Use existing depth/time model"));
    mAttachCB( used2tmbox_->activated, uiTieWinMGRDlg::d2TSelChg );
    used2tmbox_->attach( alignedBelow, logsfld_ );

    const char** corrs = WellTie::Setup::CorrTypeNames();
    cscorrfld_ = new uiLabeledComboBox( logsgrp, corrs,
					WellTie::Setup::sCSCorrType());
    cscorrfld_->attach( alignedBelow, used2tmbox_ );
    logsgrp->setHAlignObj( cscorrfld_ );

    sep = new uiSeparator( this, "Logs2Wavelt Sep" );
    sep->attach( stretchedBelow, logsgrp );

    uiMultiSynthSeisSel::Setup sssu( "Reference wavelet" );
    sssu.withps_ = false;

    wvltfld_ = new uiMultiSynthSeisSel( this, sssu );
    wvltfld_->attach( alignedBelow, wellfld_ );
    wvltfld_->attach( ensureBelow, sep );

    mAttachCB( postFinalize(), uiTieWinMGRDlg::initDlg );
}


uiTieWinMGRDlg::~uiTieWinMGRDlg()
{
    detachAllNotifiers();
    delWins();
    delete &wtsetup_;
    delete &elpropsel_;
}


void uiTieWinMGRDlg::initDlg( CallBacker* )
{
    wellSelChg( nullptr );
}


void uiTieWinMGRDlg::delWins()
{
    while ( !welltiedlgset_.isEmpty() )
    {
	uiTieWin* tiedlg = welltiedlgset_.pop();
	closeAndZeroPtr( tiedlg );
    }
}


bool uiTieWinMGRDlg::selIs2D() const
{
    if ( typefld_ )
	return !typefld_->getIntValue();

    return SI().has2D();
}

#define mErrRet(s) { if ( !s.isEmpty() && cb ) uiMSG().error(s); return; }

void uiTieWinMGRDlg::wellSelChg( CallBacker* cb )
{
    const IOObj* wellobj = wellfld_->ioobj(true);
    if ( !wellobj ) return;
    const char* wllfilenm = Well::odIO::getMainFileName( *wellobj );
    const MultiID& wellid = wellobj->key();

    Well::LoadReqs lreqs(Well::Trck, Well::Mrkrs, Well::LogInfos);
    wd_ = Well::MGR().get( wellid, lreqs );
    if ( !wd_ ) mErrRet( uiStrings::phrCannotRead(mJoinUiStrs(sWell().toLower(),
							    sData().toLower())))

    logsfld_->setWellID( wellid );
    BufferStringSet notokpropnms;
    if ( !logsfld_->setAvailableLogs(wd_->logs(),notokpropnms) )
    {
	if ( cb )
	{
	    BufferString logstr( notokpropnms.size()>1 ? "logs " : "log " );
	    uiString errmsg = tr("No valid %1 found for %2. Create missing %3"
				 " here or import through well manager")
			    .arg(logstr.buf())
			    .arg(notokpropnms.getDispString())
			    .arg(logstr.buf());
	    uiMSG().error( errmsg );
	}

	return;
    }

    const bool canuseexistingd2t = wd_->d2TModel() && !mIsUnvalidD2TM((wd_));
    used2tmbox_->display( canuseexistingd2t );
    used2tmbox_->setChecked( canuseexistingd2t );

    getSetup( wllfilenm );
}


void uiTieWinMGRDlg::typeSelChg( CallBacker* )
{
    const bool is2d = selIs2D();
    if ( seis3dfld_ )
	seis3dfld_->display( !is2d );

    if ( seis2dfld_ )
	seis2dfld_->display( is2d );

    if ( seislinefld_ )
	seislinefld_->display( is2d );
}


void uiTieWinMGRDlg::seisSelChg( CallBacker* cb )
{
    const bool is2d = selIs2D();
    mDynamicCastGet( uiSeisSel*, seisfld, is2d ? seis2dfld_ : seis3dfld_ );
    if ( !seisfld )
	mErrRet(uiStrings::phrSelect(tr("%1 type")
					.arg(uiStrings::sSeismic().toLower())))

    if ( seisfld->isChecked() )
    {
	const MultiID& seisid = seisfld->key();
	if ( seisid.isUdf() )
	    mErrRet(uiStrings::phrSelect(uiStrings::phrJoinStrings(
		    uiStrings::sInput().toLower(),
		    uiStrings::sSeismic().toLower(),
		    uiStrings::sData().toLower())))

	if ( is2d && seislinefld_ )
	    seislinefld_->setDataSet( seisid );
    }
}


void uiTieWinMGRDlg::seis2DCheckChg( CallBacker* )
{
    seislinefld_->display( seis2dfld_->isChecked() );
}


void uiTieWinMGRDlg::d2TSelChg( CallBacker* )
{
    const bool useexistingmdl = used2tmbox_->isChecked();
    const bool havecs = wd_ && wd_->haveCheckShotModel();
    cscorrfld_->display( !useexistingmdl && havecs );
}


#undef mErrRet
#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }


void uiTieWinMGRDlg::getSetup( const char* nm )
{
    WellTie::Reader wtr( nm );
    wtr.getWellTieSetup( wtsetup_ );
    getSeismicInSetup();
    getVelLogInSetup();
    getDenLogInSetup();

    if ( !wtsetup_.wvltid_.isUdf() )
	wvltfld_->setWavelet( wtsetup_.wvltid_ );

    if ( !wtsetup_.useexistingd2tm_ )
    {
	used2tmbox_->setChecked(false);
	const bool havecs = wd_ && wd_->haveCheckShotModel();
	cscorrfld_->display( havecs );
	cscorrfld_->box()->setCurrentItem( wtsetup_.corrtype_ );
    }

    d2TSelChg( nullptr );
}


bool uiTieWinMGRDlg::getSeismicInSetup()
{
    if ( !wtsetup_.seisid_.isUdf() )
    {
	PtrMan<IOObj> ioobj = IOM().get( wtsetup_.seisid_ );
	if ( !ioobj )
	{
	    uiString errmsg = tr("The Seismic data in the stored setup "
				 "does not exist anymore.");
	    mErrRet( errmsg );
	}

	const bool idinsetupis2d = !seisIDIs3D( wtsetup_.seisid_ );
	const bool surveyhastype = idinsetupis2d ? SI().has2D() : SI().has3D();
	if ( !surveyhastype )
	{
	    uiString errmsg  = tr("Stored setup contains seismic of another "
				  "type\nthan the survey.\n"
				  "Change the survey type to 2D/3D.\n"
				  "Or select a new dataset.");
	    mErrRet( errmsg );
	}

	if ( typefld_ )
	{
	    typefld_->setValue( !idinsetupis2d );
	    typeSelChg(0);
	}

	mDynamicCastGet( uiSeisSel*, seisfld,
			 idinsetupis2d ? seis2dfld_ : seis3dfld_ );
	if ( seisfld )
	{
	    seisfld->setInput( wtsetup_.seisid_ );
	    if ( seisfld->key().isUdf() )
	    {
		seisfld->setEmpty();
		mErrRet(tr("Cannot restore the seismic data from the setup"))
	    }

	    const MultiID& actualseisid = seisfld->key(true);
	    const bool commitfailed = actualseisid != wtsetup_.seisid_;
	    if ( commitfailed )
		seisfld->setInput( actualseisid );

	    if ( idinsetupis2d && seislinefld_ &&
		 !wtsetup_.linenm_.isEmpty() )
	    {
		seislinefld_->setDataSet( actualseisid );
		if ( !commitfailed )
		    seislinefld_->setInput( wtsetup_.linenm_ );
	    }
	}
    }

    return true;
}


#define mPwaveIdx 1
bool uiTieWinMGRDlg::getVelLogInSetup() const
{
    if ( wtsetup_.vellognm_.isEmpty() )
	return true;

    if ( !wd_ )
    {
	mErrRet(uiStrings::phrCannotFind(
		mJoinUiStrs(sWell().toLower(),sData().toLower())))
    }

    const Well::Log* vp = wd_->getLog( wtsetup_.vellognm_ );
    if ( !vp )
    {
	uiString errmsg = tr("Cannot retrieve the velocity log %1"
			     " stored in the setup.")
			.arg(wtsetup_.vellognm_);
	mErrRet( errmsg );
    }

    const Mnemonic* logmn = vp->mnemonic();
    const UnitOfMeasure* velpuom = vp->unitOfMeasure();
    const bool reverted = wtsetup_.issonic_;
    logsfld_->setLog( logmn, wtsetup_.vellognm_, reverted, velpuom,
		      mPwaveIdx );

    return true;
}


#define mDensityIdx 0
bool uiTieWinMGRDlg::getDenLogInSetup() const
{
    if ( wtsetup_.denlognm_.isEmpty() )
	return true;

    if ( !wd_ )
    {
	mErrRet(uiStrings::phrCannotFind(
		mJoinUiStrs(sWell().toLower(),sData().toLower())))
    }

    const Well::Log* den = wd_->getLog( wtsetup_.denlognm_ );
    if ( !den )
    {
	uiString errmsg = tr("Cannot retrieve the density log %1"
			     " stored in the setup.")
			.arg(toUiString(wtsetup_.denlognm_));
	mErrRet( errmsg );
    }

    const Mnemonic* logmn = den->mnemonic();
    const UnitOfMeasure* denuom = den->unitOfMeasure();
    const bool reverted = false;
    logsfld_->setLog( logmn, wtsetup_.denlognm_, reverted, denuom,mDensityIdx );

    return true;
}


void uiTieWinMGRDlg::saveWellTieSetup( const MultiID& key,
				      const WellTie::Setup& wts ) const
{
    WellTie::Writer wtr( Well::odIO::getMainFileName(key) );
    if ( !wtr.putWellTieSetup( wts ) )
	uiMSG().error( uiStrings::phrCannotWrite(tr("parameters")) );
}



#undef mErrRet
#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }


bool uiTieWinMGRDlg::initSetup()
{
    if ( !wellfld_->commitInput() )
	mErrRet(uiStrings::phrSelect(tr("a valid well")))

    const MultiID& wellid = wellfld_->ctxtIOObj().ioobj_->key();

    Well::LoadReqs lreqs(Well::Trck, Well::Mrkrs, Well::LogInfos);
    wd_ = Well::MGR().get( wellid, lreqs );
    if ( !wd_ )
	mErrRet(uiStrings::phrCannotRead(mJoinUiStrs(
					 sWell().toLower(),sData().toLower())))

    for ( int idx=0; idx<welltiedlgset_.size(); idx++ )
    {
	uiTieWin* win = welltiedlgset_[idx];
	if ( win->welltieSetup().wellid_ == wellid )
	    mErrRet( tr("A window with this well is already opened") )
    }
    wtsetup_.wellid_ = wellid;

    const bool is2d = selIs2D();
    mDynamicCastGet( uiSeisSel*, seisfld, is2d ? seis2dfld_ : seis3dfld_ );
    if ( !seisfld )
	mErrRet( uiStrings::phrSelect(tr("a seismic type")) )

    if ( seisfld->isChecked() )
    {
	const MultiID& seisid = seisfld->key();
	if ( seisid.isUdf() )
	    mErrRet(uiStrings::phrSelect(uiStrings::phrInput(
					  mJoinUiStrs(sWell(),sData()))))
	    // msg required because the seismic is optional

	wtsetup_.seisid_ = seisid;
	wtsetup_.seisnm_ = IOM().nameOf( seisid );
	if ( is2d )
	    wtsetup_.linenm_ = seislinefld_->getInput();
	else
	    wtsetup_.linenm_.setEmpty();
    }
    else
    {
	wtsetup_.seisid_.setUdf();
	if ( is2d )
	    wtsetup_.linenm_.setEmpty();
    }

    if ( !logsfld_->isOK() )
	mErrRet( tr("Cannot select appropriate logs") )

    BufferString lognm;
    bool isrev;
    const UnitOfMeasure* uom;
    const Mnemonic* mn = &elpropsel_.get( mDensityIdx )->mn();
    if ( !logsfld_->getLog(*mn,lognm,isrev,uom,mDensityIdx) )
	mErrRet( uiStrings::phrCannotFind(
				tr("the density in the log selection list")) )

    const Well::Log* den = wd_->getLog( lognm );
    if ( !den )
	mErrRet( uiStrings::phrCannotExtract(tr("this density log")) )

    wtsetup_.denlognm_ = lognm;
    if ( !den->unitOfMeasure() )
    {
	if ( !uom )
	    mErrRet( uiStrings::phrSelect(tr("a unit for the density log")) )

	const_cast<Well::Log*>( den )->setUnitOfMeasure( uom );
	//TODO: Write to DB
    }

    mn = &elpropsel_.get( mPwaveIdx )->mn();
    if ( !logsfld_->getLog(*mn,lognm,isrev,uom,mPwaveIdx) )
	mErrRet( uiStrings::phrCannotFind(
				    tr("the Pwave in the log selection list")) )

    const Well::Log* vp = wd_->getLog( lognm );
    if ( !vp )
	mErrRet( uiStrings::phrCannotExtract(tr("this velocity log")) )

    wtsetup_.vellognm_ = lognm;
    wtsetup_.issonic_  = isrev;
    if ( !vp->unitOfMeasure() )
    {
	if ( !uom )
	    mErrRet( uiStrings::phrSelect(tr("a unit for the velocity log")) )

	const_cast<Well::Log*>( vp )->setUnitOfMeasure( uom );
	//TODO: Write to DB
    }

    wtsetup_.useexistingd2tm_ = used2tmbox_->isChecked();
    if ( wtsetup_.useexistingd2tm_ )
	wtsetup_.corrtype_ = WellTie::Setup::None;
    else
	WellTie::Setup::CorrTypeDef().parse( cscorrfld_->box()->text(),
				       wtsetup_.corrtype_ );

    const uiRetVal uirv = wvltfld_->isOK();
    if ( !uirv.isOK() )
	mErrRet(uirv)

    wtsetup_.wvltid_ = wvltfld_->getWaveletID();

    wtsetup_.commitDefaults();
    if ( saveButtonChecked() )
	saveWellTieSetup( wtsetup_.wellid_, wtsetup_ );

    return true;
}


bool uiTieWinMGRDlg::acceptOK( CallBacker* )
{
    if ( !initSetup() )
	return false;

    Server* server = new Server( wtsetup_ );
    if ( !server->isOK() )
	{ uiMSG().error( server->errMsg() ); delete server; return false; }

    if ( wtsetup_.corrtype_ == WellTie::Setup::UserDefined )
    {
	uiCheckShotEdit dlg( this, *server );
	if ( !dlg.go() )
	    { delete server; return false; }
    }

    WellTie::uiTieWin* wtdlg = new WellTie::uiTieWin( this, *server );
    wtdlg->setDeleteOnClose( true );
    welltiedlgset_ += wtdlg;
    wtdlg->windowClosed.notify( mCB(this,uiTieWinMGRDlg,wellTieDlgClosed) );

    PtrMan<IOObj> ioobj = IOM().get( wtsetup_.wellid_ );
    if ( !ioobj ) return false;

    const BufferString fname( ioobj->fullUserExpr(true) );
    WellTie::Reader wtr( fname );
    IOPar* par= wtr.getIOPar( uiTieWin::sKeyWinPar() );
    if ( par ) wtdlg->usePar( *par );
    delete par;

    return false;
}


void uiTieWinMGRDlg::wellTieDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(WellTie::uiTieWin*,tiewin,cb);
    const int idx = welltiedlgset_.indexOf( tiewin );
    if ( !tiewin || idx<0 ) return;

    const MultiID wellid = tiewin->welltieSetup().wellid_;
    WellTie::Writer wtr( Well::odIO::getMainFileName(wellid) );
    IOPar par; tiewin->fillPar( par );
    wtr.putIOPar( par, uiTieWin::sKeyWinPar() );

    welltiedlgset_.removeSingle( idx );
}


bool uiTieWinMGRDlg::seisIDIs3D( MultiID seisid ) const
{
    PtrMan<IOObj> ioobj = IOM().get( seisid );
    if ( !ioobj )
	return true;

    const bool is2D = SeisTrcTranslator::is2D( *ioobj, true );
    const bool islineset = SeisTrcTranslator::isLineSet(*ioobj);

    return !is2D && !islineset;
}


const MultiID& uiTieWinMGRDlg::getWellId() const
{
    return wtsetup_.wellid_;
}
} // namespace WellTie
