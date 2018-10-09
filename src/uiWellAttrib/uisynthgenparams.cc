/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Dec 2014
________________________________________________________________________

-*/

#include "uisynthgenparams.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uisplitter.h"
#include "uispinbox.h"
#include "uisynthtorealscale.h"
#include "uitoolbutton.h"
#include "uiwaveletsel.h"

#include "stratsynthdatamgr.h"
#include "waveletmanager.h"

#include "od_helpids.h"


class uiSynthCorrAdvancedDlg;

class uiSynthCorrectionsGrp : public uiGroup
{ mODTextTranslationClass(uiSynthCorrectionsGrp);
public:

				uiSynthCorrectionsGrp(uiParent*);
				~uiSynthCorrectionsGrp();

    bool			wantNMOCorr() const;
    void			setValues(bool,float mutelen,float stretchlim);

    float			mutelen_;
    float			stretchmutelim_;

protected:

    uiCheckBox*			nmobox_;
    uiButton*			nmoparsbut_;

    void			advancedPush(CallBacker*);
    void			nmoChgCB(CallBacker*);

};



#define mTypFldNotif typefld_->selectionChanged
#define mNameFldNotif namefld_->valuechanging
#define mWvltFldFldNotif wvltfld_->selectionDone


uiSynthGenParams::uiSynthGenParams( uiParent* p, const DataMgr& mgr )
    : uiGroup(p)
    , mgr_(mgr)
    , nameChanged(this)
{
    auto* typlcb = new uiLabeledComboBox( this, tr("Synthetic type") );
    typefld_ = typlcb->box();

    wvltfld_ = new uiWaveletIOObjSel( this );
    wvltfld_->attach( alignedBelow, typlcb );
    wvltscalebut_ = new uiToolButton( this, "wavelet_scale",
				      tr("Scale this Wavelet"),
				      mCB(this,uiSynthGenParams,scaleWvltCB) );
    wvltscalebut_->attach( rightOf, wvltfld_ );

    uiGroup* parsgrp = createGroups();
    parsgrp->attach( alignedBelow, wvltfld_ );

    auto* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, parsgrp );

    namefld_ = new uiGenInput( this, uiStrings::sName() );
    namefld_->setElemSzPol( uiObject::WideVar );
    namefld_->attach( alignedBelow, wvltfld_ );
    namefld_->attach( ensureBelow, sep );

    setHAlignObj( wvltfld_ );
    postFinalise().notify( mCB(this,uiSynthGenParams,initWin) );
}


uiGroup* uiSynthGenParams::createGroups()
{
    uiGroup* parsgrp = new uiGroup( this, "Pars group" );

    zeroofssgrp_ = new uiGroup( parsgrp, "Zerooffs group" );
    dointernalmultiplesfld_ = new uiGenInput( zeroofssgrp_,
			tr("Compute internal multiples"), BoolInpSpec(false) );
    auto* lsb = new uiLabeledSpinBox( zeroofssgrp_,
			tr("Surface reflection coefficient") );
    surfreflcoeffld_ = lsb->box();
    const StepInterval<double> rcintv(  -1, 1, 0.1 );
    surfreflcoeffld_->setInterval( rcintv );
    surfreflcoeffld_->setValue( rcintv.stop );
    surfreflcoeffld_->setNrDecimals( 1 );
    lsb->attach( alignedBelow, dointernalmultiplesfld_ );
    zeroofssgrp_->setHAlignObj( dointernalmultiplesfld_ );

    prestackgrp_ = new uiGroup( parsgrp, "Prestack group" );
    uiRayTracer1D::Setup rtsu;
    rtsu.dooffsets( true ).convertedwaves( true ).showzerooffsetfld( false );
    rtsel_ = new uiRayTracerSel( prestackgrp_, rtsu );
    uisynthcorrgrp_ = new uiSynthCorrectionsGrp( prestackgrp_ );
    uisynthcorrgrp_->attach( alignedBelow, rtsel_ );
    prestackgrp_->setHAlignObj( uisynthcorrgrp_ );
    prestackgrp_->attach( alignedWith, zeroofssgrp_ );

    pspostprocgrp_ = new uiGroup( parsgrp, "PS Post proc group" );
    auto* lcb = new uiLabeledComboBox( pspostprocgrp_, tr("Input Prestack") );
    psinpfld_ = lcb->box();
    IntInpIntervalSpec angspec( false );
    angspec.setLimits( Interval<int>(0,90) );
    angspec.setDefaultValue( Interval<int>(0,30) );
    angleinpfld_ = new uiGenInput( pspostprocgrp_,
		tr("Angle Range").withUnit(uiStrings::sDeg()), angspec );
    angleinpfld_->attach( alignedBelow, lcb );
    pspostprocgrp_->attach( alignedWith, zeroofssgrp_ );
    pspostprocgrp_->setHAlignObj( angleinpfld_ );

    parsgrp->setHAlignObj( zeroofssgrp_ );
    return parsgrp;
}


uiSynthGenParams::~uiSynthGenParams()
{
    detachAllNotifiers();
}


void uiSynthGenParams::initWin( CallBacker* )
{
    fillTypeFld();
    updUi();
    mAttachCB( mTypFldNotif, uiSynthGenParams::typeChgCB );
    mAttachCB( mNameFldNotif, uiSynthGenParams::nameChgCB );
    mAttachCB( mWvltFldFldNotif, uiSynthGenParams::waveletSelCB );
}


void uiSynthGenParams::typeChgCB( CallBacker* )
{
    updUi();
}


void uiSynthGenParams::nameChgCB( CallBacker* )
{
    nameChanged.trigger();
}


void uiSynthGenParams::waveletSelCB( CallBacker* )
{
    updWvltScaleFldDisp();
}


void uiSynthGenParams::scaleWvltCB( CallBacker* )
{
    const auto wvltid = wvltfld_->key( true );
    if ( !wvltid.isValid() )
	return;
    const auto res = uiMSG().ask2D3D( tr("Use 2D or 3D data?"), true );
    if ( res < 0 )
	return;
    uiSynthToRealScale dlg( this, mgr_, wvltid, res==1, Strat::Level::ID() );
    if ( !dlg.go() )
	return;

    wvltfld_->setInput( dlg.scaledWvltID() );
    updWvltScaleFldDisp();
}


BufferString uiSynthGenParams::getName() const
{
    BufferString ret( namefld_->text() );
    ret.trimBlanks();
    return ret;
}


uiSynthGenParams::SynthType uiSynthGenParams::typeFromFld() const
{
    int selidx = typefld_->currentItem();
    if ( selidx < 0 )
	selidx = 0;
    return synthtypes_[selidx];
}


void uiSynthGenParams::typeToFld( SynthType typ )
{
    int newselidx = synthtypes_.indexOf( typ );
    if ( newselidx < 0 )
	newselidx = 0;
    typefld_->setCurrentItem( newselidx );
}


void uiSynthGenParams::fillTypeFld()
{
    synthtypes_.setEmpty();
    synthtypes_.add( SynthSeis::ZeroOffset );
    synthtypes_.add( SynthSeis::PreStack );
    if ( mgr_.haveOfType(SynthSeis::PreStack) )
    {
	synthtypes_.add( SynthSeis::AngleStack );
	synthtypes_.add( SynthSeis::AVOGradient );
    }

    const auto curseltyp = typeFromFld();

    NotifyStopper ns( mTypFldNotif );
    typefld_->setEmpty();
    for ( auto typ : synthtypes_ )
	typefld_->addItem( SynthSeis::toUiString(typ) );

    typeToFld( curseltyp );
}


void uiSynthGenParams::updUi()
{
    const auto type = typeFromFld();

    const bool ispspostproc = GenParams::isPSPostProc( type );
    zeroofssgrp_->display( GenParams::isZeroOffset(type) );
    prestackgrp_->display( GenParams::isPS(type) );
    pspostprocgrp_->display( ispspostproc );

    if ( ispspostproc )
    {
	const BufferString seltxt( psinpfld_->text() );
	psinpfld_->setEmpty();
	BufferStringSet nms;
	mgr_.getNames( nms, DataMgr::OnlyPS );
	psinpfld_->addItems( nms );
	psinpfld_->setCurrentItem( seltxt );
    }

    updWvltScaleFldDisp();
}


void uiSynthGenParams::updWvltScaleFldDisp()
{
    const auto wvltid = wvltfld_->key( true );
    wvltscalebut_->setSensitive(
			wvltid.isValid() && !WaveletMGR().isScaled(wvltid) );
}


void uiSynthGenParams::setByName( const char* nm )
{
    const auto id = mgr_.find( nm );
    set( mgr_.getGenParams(id) );
}


void uiSynthGenParams::set( const GenParams& gp )
{
    NotifyStopper nstyp( mTypFldNotif );
    NotifyStopper nsnm( mNameFldNotif );

    typeToFld( gp.type_ );
    wvltfld_->setInput( gp.wvltid_ );
    namefld_->setText( gp.name_ );

    const IOPar& iop = gp.raypars_;
    if ( gp.isZeroOffset() )
    {
	bool dointernalmultiples = false; float surfreflcoeff = 1.f;
	iop.getYN( SynthSeis::GenBase::sKeyInternal(), dointernalmultiples );
	iop.get( SynthSeis::GenBase::sKeySurfRefl(), surfreflcoeff );
	dointernalmultiplesfld_->setValue( dointernalmultiples );
	surfreflcoeffld_->setValue( surfreflcoeff );
    }
    else if ( gp.isPS() )
    {
	rtsel_->usePar( iop );
	bool donmo = true;
	iop.getYN( SynthSeis::GenBase::sKeyNMO(), donmo );

	float stretchlimit = SynthSeis::GenBase::cStdStretchLimit();
	iop.get( SynthSeis::GenBase::sKeyStretchLimit(), stretchlimit );
	float mutelen = SynthSeis::GenBase::cStdMuteLength();
	iop.get( SynthSeis::GenBase::sKeyMuteLength(), mutelen );
	uisynthcorrgrp_->setValues( donmo, mutelen, stretchlimit );
    }
    else if ( gp.isPSPostProc() )
    {
	psinpfld_->setText( gp.inpsynthnm_ );
	angleinpfld_->setValue( gp.anglerg_ );
    }

    fillTypeFld();
    updUi();
}


void uiSynthGenParams::get( GenParams& gp ) const
{
    gp.type_ = typeFromFld();
    gp.name_ = getName();
    gp.wvltid_ = wvltfld_->key( true );

    IOPar& iop = gp.raypars_;
    iop.setEmpty();
    if ( gp.isZeroOffset() )
    {
	RayTracer1D::setIOParsToZeroOffset( iop );
	iop.setYN( SynthSeis::GenBase::sKeyFourier(), true );
	const bool dointernal = dointernalmultiplesfld_->getBoolValue();
	iop.setYN( SynthSeis::GenBase::sKeyInternal(), dointernal );
	if ( dointernal )
	{
	    iop.set( SynthSeis::GenBase::sKeySurfRefl(),
		     surfreflcoeffld_->getFValue() );
	}
    }
    else if ( gp.isPS() )
    {
	rtsel_->fillPar( iop );
	iop.setYN( SynthSeis::GenBase::sKeyFourier(), true );
	const bool donmo = uisynthcorrgrp_->wantNMOCorr();
	iop.setYN( SynthSeis::GenBase::sKeyNMO(), donmo );
	if ( donmo )
	{
	    iop.set( SynthSeis::GenBase::sKeyStretchLimit(),
		       uisynthcorrgrp_->stretchmutelim_ );
	    iop.set( SynthSeis::GenBase::sKeyMuteLength(),
		       uisynthcorrgrp_->mutelen_ );
	}
    }
    else if ( gp.isPSPostProc() )
    {
	gp.inpsynthnm_.set( psinpfld_->text() );
	gp.anglerg_ = angleinpfld_->getIInterval();
    }
}


class uiSynthCorrAdvancedDlg : public uiDialog
{ mODTextTranslationClass(uiSynthCorrAdvancedDlg);
public:

uiSynthCorrAdvancedDlg( uiParent* p, float& sml, float& ml )
    : uiDialog( p, uiDialog::Setup(tr("Synthetic Corrections advanced options"),
	tr("Advanced options"), mODHelpKey(mSynthCorrAdvancedDlgHelpID)))
    , smlimit_(sml)
    , mutelen_(ml)
{
    auto* lsb = new uiLabeledSpinBox( this,
				      tr("Stretch mute limit").withUnit("%") );
    smlimitfipctld_ = lsb->box();
    smlimitfipctld_->setInterval( 1, 500, 1 );
    const float smlimitpct = 100.f * smlimit_;
    smlimitfipctld_->setValue( mNINT32(smlimitpct) );

    const float mutelenms = mIsUdf(mutelen_) ? mutelen_ : 1000.f * mutelen_;
    mutemsfld_ = new uiGenInput( this, tr("Mute taper-length").withUnit("ms"),
				  FloatInpSpec(mutelenms) );
    mutemsfld_->attach( alignedBelow, lsb );
}

bool acceptOK()
{
    const float mlms = mutemsfld_->getFValue();
    if ( mIsUdf(mlms) || mlms < 0 )
	{ uiMSG().error( tr("Invalid mute length") ); return false; }

    mutelen_ = mlms * 0.001f;
    smlimit_ = smlimitfipctld_->getFValue() * 0.01f;
    return true;
}

    float&		mutelen_;
    float&		smlimit_;

    uiGenInput*		mutemsfld_;
    uiSpinBox*		smlimitfipctld_;

};



uiSynthCorrectionsGrp::uiSynthCorrectionsGrp( uiParent* p )
    : uiGroup( p, "Synth corrections parameters" )
    , stretchmutelim_(SynthSeis::GenBase::cStdStretchLimit())
    , mutelen_(SynthSeis::GenBase::cStdMuteLength())
{
    nmobox_ = new uiCheckBox( this, tr("Apply NMO corrections") );
    mAttachCB( nmobox_->activated, uiSynthCorrectionsGrp::nmoChgCB );

    CallBack parscb = mCB(this,uiSynthCorrectionsGrp,advancedPush);
    nmoparsbut_ = new uiPushButton( this, uiStrings::sParameter(mPlural),
				    parscb, false );
    nmoparsbut_->setIcon( "settings" );
    nmoparsbut_->attach( rightOf, nmobox_ );

    setHAlignObj( nmobox_ );
}


uiSynthCorrectionsGrp::~uiSynthCorrectionsGrp()
{
    detachAllNotifiers();
}


void uiSynthCorrectionsGrp::nmoChgCB( CallBacker* )
{
    nmoparsbut_->display( wantNMOCorr() );
}


bool uiSynthCorrectionsGrp::wantNMOCorr() const
{
    return nmobox_->isChecked();
}


void uiSynthCorrectionsGrp::advancedPush( CallBacker* )
{
    uiSynthCorrAdvancedDlg dlg( this, stretchmutelim_, mutelen_ );
    dlg.go();
}


void uiSynthCorrectionsGrp::setValues( bool donmo, float mlen, float slim )
{
    nmobox_->setChecked( donmo );
    stretchmutelim_ = slim;
    mutelen_ = mlen;
}
