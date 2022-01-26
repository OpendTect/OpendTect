/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		Dec 2014
________________________________________________________________________

-*/

#include "uisynthseis.h"

#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseiswvltsel.h"
#include "uispinbox.h"
#include "uisplitter.h"

#include "od_helpids.h"
#include "synthseis.h"
#include "zdomain.h"


#define mErrRet(s,act) \
{ uiMsgMainWinSetter mws(mainwin()); if (!s.isEmpty()) uiMSG().error(s); act; }

uiSynthSeisGrp::uiSynthSeisGrp( uiParent* p, const uiRayTracer1D::Setup& su )
    : uiGroup(p)
    , parsChanged(this)
{
    wvltfld_ = new uiSeisWaveletSel( this, "Wavelet", true, true, true );
    mAttachCB( wvltfld_->newSelection, uiSynthSeisGrp::parsChangedCB );
    wvltfld_->setFrame( false );

    rtsel_ = new uiRayTracerSel( this, su );
    mAttachCB( rtsel_->parsChanged, uiSynthSeisGrp::parsChangedCB );
    rtsel_->attach( alignedBelow, wvltfld_ );

    if ( su.doreflectivity_ )
    {
	internalmultiplebox_ = new uiCheckBox( this,
				tr("Compute internal multiples") );
	/* internalmultiplebox_->attach( alignedBelow, uisynthcorrgrp_ );
	   At the moment this option is only available for Zero Offset,
	   thus compressing the layout	 */
	internalmultiplebox_->attach( alignedBelow, wvltfld_ );
	mAttachCB( internalmultiplebox_->activated,
		   uiSynthSeisGrp::parsChangedCB );
	surfreflcoeffld_ = new uiLabeledSpinBox( this,
				tr("Surface reflection coefficient"), 1 );
	surfreflcoeffld_->box()->setInterval( -1., 1., 0.1 );
	surfreflcoeffld_->box()->setValue( 1. );
	surfreflcoeffld_->attach( alignedBelow, internalmultiplebox_ );
	mAttachCB( surfreflcoeffld_->box()->valueChanged,
		   uiSynthSeisGrp::parsChangedCB );
    }

    uisynthcorrgrp_ = new uiSynthCorrectionsGrp( this );
    uisynthcorrgrp_->attach( alignedBelow, rtsel_ );

    mAttachCB( uisynthcorrgrp_->nmoparsChanged, uiSynthSeisGrp::parsChangedCB);
    setHAlignObj( wvltfld_ );
}


uiSynthSeisGrp::~uiSynthSeisGrp()
{
    detachAllNotifiers();
}


void uiSynthSeisGrp::parsChangedCB( CallBacker* )
{
    parsChanged.trigger();
}


void uiSynthSeisGrp::updateFieldDisplay()
{
    const bool iszerooofset = rtsel_->current()->isZeroOffset();
    if ( !rtsel_->current()->hasZeroOffsetFld() )
    { // cannot set zerooffset from outside e.g uiSynthGenDlg
	rtsel_->display( !iszerooofset );
    }

    uisynthcorrgrp_->display( !iszerooofset );
    if ( internalmultiplebox_ )
    {
	internalmultiplebox_->display( iszerooofset );
	surfreflcoeffld_->display( iszerooofset );
    }
}


void uiSynthSeisGrp::setRayTracerType( const char* type )
{
    rtsel_->setCurrentType( type );
}


const char* uiSynthSeisGrp::getWaveletName() const
{
    return wvltfld_->getWaveletName();
}


void uiSynthSeisGrp::setWavelet( const char* wvltnm )
{
    wvltfld_->setInput( wvltnm );
}

#define mIsZeroOffset( offsets ) \
    (offsets.isEmpty() || (offsets.size()==1 && mIsZero(offsets[0],mDefEps)))

void uiSynthSeisGrp::usePar( const IOPar& iopar )
{
    TypeSet<float> offsets;
    iopar.get( RayTracer1D::sKeyOffset(), offsets );
    const bool iszerooofset = mIsZeroOffset(offsets);
    MultiID waveletid;
    if ( iopar.get(sKey::WaveletID(),waveletid) )
	wvltfld_->setInput( waveletid );

    if ( iszerooofset )
    {
	bool dointernalmultiples = false; float surfreflcoeff = 1.f;
/*	iopar.getYN( Seis::SynthGenBase::sKeyInternal(), dointernalmultiples );
	iopar.get( Seis::SynthGenBase::sKeySurfRefl(), surfreflcoeff );*/
	internalmultiplebox_->setChecked( dointernalmultiples );
	surfreflcoeffld_->box()->setValue( surfreflcoeff );
    }
    else
    {
	bool donmo = true;
	iopar.getYN( Seis::SynthGenBase::sKeyNMO(), donmo );

	float mutelen = Seis::SynthGenBase::cStdMuteLength();
	iopar.get( Seis::SynthGenBase::sKeyMuteLength(), mutelen );
	if ( !mIsUdf(mutelen) )
	   mutelen = mutelen *ZDomain::Time().userFactor();

	float stretchlimit = Seis::SynthGenBase::cStdStretchLimit();
	iopar.get( Seis::SynthGenBase::sKeyStretchLimit(), stretchlimit );
	uisynthcorrgrp_->setValues( donmo, mutelen, mToPercent( stretchlimit ));
    }

    rtsel_->usePar( iopar );
    updateFieldDisplay();
}


void uiSynthSeisGrp::fillPar( IOPar& iopar ) const
{
    iopar.setEmpty();

    rtsel_->fillPar( iopar );
    iopar.set( sKey::WaveletID(), wvltfld_->getID() );

    const bool iszeroffset = rtsel_->current()->isZeroOffset();
    if ( iszeroffset )
    {
	RayTracer1D::setIOParsToZeroOffset( iopar );
/*	const bool dointernal = internalmultiplebox_->isChecked();
	const float coeff = surfreflcoeffld_->box()->getFValue();
	iopar.setYN( Seis::SynthGenBase::sKeyInternal(), dointernal );
	iopar.set( Seis::SynthGenBase::sKeySurfRefl(), coeff );*/
    }
    else
    {
	const bool donmo = iszeroffset ? false
				       : uisynthcorrgrp_->wantNMOCorr();
	iopar.setYN( Seis::SynthGenBase::sKeyNMO(), donmo );
	iopar.set( Seis::SynthGenBase::sKeyMuteLength(),
	     uisynthcorrgrp_->getMuteLength() / ZDomain::Time().userFactor() );
	iopar.set( Seis::SynthGenBase::sKeyStretchLimit(),
		   mFromPercent( uisynthcorrgrp_->getStrechtMutePerc()) );
    }
}


class uiSynthCorrAdvancedDlg : public uiDialog
{ mODTextTranslationClass(uiSynthCorrAdvancedDlg);
    public:
				uiSynthCorrAdvancedDlg(uiParent*);

    uiGenInput*			stretchmutelimitfld_;
    uiGenInput*			mutelenfld_;

    protected:

    bool			acceptOK(CallBacker*) override;
};


uiSynthCorrectionsGrp::uiSynthCorrectionsGrp( uiParent* p )
    : uiGroup( p, "Synth corrections parameters" )
    , nmoparsChanged(this)
{
    nmofld_ = new uiGenInput( this, tr("Apply NMO corrections"),
			      BoolInpSpec(true) );
    nmofld_->setValue( true );
    mAttachCB( nmofld_->valuechanged, uiSynthCorrectionsGrp::nmoSelCB );

    CallBack cbadv = mCB(this,uiSynthCorrectionsGrp,getAdvancedPush);
    advbut_ = new uiPushButton( this, uiStrings::sAdvanced(), cbadv, false );
    advbut_->attach( rightTo, nmofld_ );

    uiscadvdlg_ = new uiSynthCorrAdvancedDlg( this );
    setHAlignObj( nmofld_ );

    mAttachCB( postFinalise(), uiSynthCorrectionsGrp::initGrp );
}


uiSynthCorrectionsGrp::~uiSynthCorrectionsGrp()
{
    detachAllNotifiers();
}


void uiSynthCorrectionsGrp::initGrp( CallBacker* )
{
    advbut_->display( wantNMOCorr() );
}


void uiSynthCorrectionsGrp::nmoSelCB( CallBacker* )
{
    advbut_->display( wantNMOCorr() );
    parsChanged( nullptr );
}


void uiSynthCorrectionsGrp::parsChanged( CallBacker* )
{
    nmoparsChanged.trigger();
}


bool uiSynthCorrectionsGrp::wantNMOCorr() const
{
    return nmofld_->getBoolValue();
}


float uiSynthCorrectionsGrp::getStrechtMutePerc() const
{
    return uiscadvdlg_->stretchmutelimitfld_->getFValue();
}


float uiSynthCorrectionsGrp::getMuteLength() const
{
    return uiscadvdlg_->mutelenfld_->getFValue();
}


void uiSynthCorrectionsGrp::getAdvancedPush( CallBacker* )
{
    const float strechmuteperc = getStrechtMutePerc();
    const float mutelength = getMuteLength();
    if ( uiscadvdlg_->go() != uiDialog::Accepted )
	return;

    if ( mIsEqual(strechmuteperc,getStrechtMutePerc(),1e-3f) &&
	 mIsEqual(mutelength,getMuteLength(),1e-3f) )
	return;

    parsChanged( nullptr );
}


void uiSynthCorrectionsGrp::setValues( bool donmo, float mutelen,
				       float stretchlim )
{
    nmofld_->setValue( donmo );
    uiscadvdlg_->mutelenfld_->setValue( mutelen );
    uiscadvdlg_->stretchmutelimitfld_->setValue( stretchlim );
}


uiSynthCorrAdvancedDlg::uiSynthCorrAdvancedDlg( uiParent* p )
    : uiDialog( p, Setup(tr("Synthetic Corrections advanced options"),
			 tr("Specify advanced options"),
			 mODHelpKey(mSynthCorrAdvancedDlgHelpID)) )
{
    FloatInpSpec inpspec;
    inpspec.setLimits( Interval<float>(1,500) );
    stretchmutelimitfld_ = new uiGenInput(this, tr("Stretch mute (%)"),
					  inpspec );

    mutelenfld_ = new uiGenInput( this, tr("Mute taper-length (ms)"),
				  FloatInpSpec() );
    mutelenfld_->attach( alignedBelow, stretchmutelimitfld_ );
}


bool uiSynthCorrAdvancedDlg::acceptOK( CallBacker* )
{
    if ( mIsUdf(mutelenfld_->getFValue() ) || mutelenfld_->getFValue()<0 )
	mErrRet( tr("The mutelength must be more than zero."), return false );

    if ( mIsUdf(stretchmutelimitfld_->getFValue()) ||
	 stretchmutelimitfld_->getFValue()<0 )
	mErrRet( tr("The stretch mute must be more than 0%"), return false );

    return true;
}
