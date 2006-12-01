/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseiswvltman.cc,v 1.10 2006-12-01 14:35:23 cvsbert Exp $
________________________________________________________________________

-*/


#include "uiseiswvltman.h"
#include "uiseiswvltimp.h"
#include "wavelet.h"
#include "ioobj.h"
#include "iostrm.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "color.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "arrayndimpl.h"

#include "uibutton.h"
#include "uiioobjsel.h"
#include "uitextedit.h"
#include "uisectiondisp.h"
#include "uigeninput.h"
#include "uimsg.h"


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Wavelet management",
                                     "Manage wavelets",
                                     "").nrstatusflds(1),
	    	   WaveletTranslatorGroup::ioContext() )
    , fdctxt_(*new FlatDisp::Context(false))
    , fda2d_(0)
    , fddata_(new FlatDisp::Data)
{
    createDefaultUI();

    uiGroup* butgrp = new uiGroup( this, "Imp/Create buttons" );
    uiPushButton* impbut = new uiPushButton( butgrp, "&Import", false );
    impbut->activated.notify( mCB(this,uiSeisWvltMan,impPush) );
    impbut->setPrefWidthInChar( 15 );
    uiPushButton* crbut = new uiPushButton( butgrp, "&Generate", false );
    crbut->activated.notify( mCB(this,uiSeisWvltMan,crPush) );
    crbut->attach( rightOf, impbut );
    crbut->setPrefWidthInChar( 15 );
    butgrp->attach( centeredBelow, selgrp );

    fdctxt_.annot_.x1name_ = "Amplitude";
    fdctxt_.annot_.x2name_ = SI().zIsTime() ? "Time" : "Depth";
    fdctxt_.ddpars_.dispvd_ = false;
    fdctxt_.ddpars_.dispwva_ = true;
    fdctxt_.ddpars_.wva_.mid_= Color( 150, 150, 100 );
    fdctxt_.ddpars_.wva_.overlap_ = -0.01;
    fdctxt_.ddpars_.wva_.clipperc_ = 0;
    wvltfld = new uiSectionDisp( this, fdctxt_, uiSize(50,mUdf(int),true) );
    wvltfld->attach( ensureRightOf, selgrp );
    wvltfld->setStretch( 1, 1 );
    wvltfld->setData( fddata_ );

    infofld->attach( ensureBelow, butgrp );
    infofld->attach( ensureBelow, wvltfld );
    selgrp->setPrefWidthInChar( 50 );
    infofld->setPrefWidthInChar( 60 );
    selChg(0);
}


uiSeisWvltMan::~uiSeisWvltMan()
{
    delete fda2d_;
    delete fddata_;
    delete &fdctxt_;
}


void uiSeisWvltMan::impPush( CallBacker* )
{
    uiSeisWvltImp dlg( this );
    if ( dlg.go() )
	selgrp->fullUpdate( dlg.selKey() );
}


class uiSeisWvltManCrWvlt : public uiDialog
{
public:

uiSeisWvltManCrWvlt( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Create Wavelet",
				 "Specify wavelet creation parameters"))
    , ctio_(*mMkCtxtIOObj(Wavelet))
{
    isrickfld = new uiGenInput( this, "Wavelet type",
				BoolInpSpec("Ricker","Sinc") );

    const float sisr = SI().zStep();
    float deffrq = 0.1 / sisr; int ideffr = mNINT(deffrq);
    if ( ideffr > 0 && mIsZero(deffrq-ideffr,1e-4) )
	deffrq = ideffr; // avoid awkward 99.999 display
    BufferString txt( "Central " );
    txt += SI().zIsTime() ? "Frequency" : "Wavenumber";
    freqfld = new uiGenInput( this, txt, FloatInpSpec(deffrq) );
    freqfld->attach( alignedBelow, isrickfld );

    const float usrsr = sisr * SI().zFactor();
    txt = "Sample interval "; txt += SI().getZUnit();
    srfld = new uiGenInput( this, txt, FloatInpSpec(usrsr) );
    srfld->attach( alignedBelow, freqfld );

    peakamplfld = new uiGenInput( this, "Peak amplitude", FloatInpSpec(1) );
    peakamplfld->attach( alignedBelow, srfld );

    ctio_.ctxt.forread = false;
    wvltfld = new uiIOObjSel( this, ctio_ );
    wvltfld->attach( alignedBelow, peakamplfld );
}

~uiSeisWvltManCrWvlt()
{
    delete ctio_.ioobj; delete &ctio_;
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool acceptOK( CallBacker* )
{
    if ( !wvltfld->commitInput(true) )
	mErrRet( "Please enter a name for the new Wavelet" );

    const float sr = srfld->getfValue();
    const float peakampl = peakamplfld->getfValue();
    const float freq = freqfld->getfValue();

    if ( mIsUdf(sr) || sr <= 0 )
	mErrRet( "The sample interval is not valid" )
    else if ( peakampl == 0 )
	mErrRet( "The peak amplitude must be non-zero" )
    else if ( mIsUdf(freq) || freq <= 0 )
	mErrRet( "The frequency must be positive" )

    const float realsr = sr / SI().zFactor();
    Wavelet wvlt( isrickfld->getBoolValue(), freq, realsr, peakampl );
    if ( !wvlt.put(ctio_.ioobj) )
    {
	mErrRet( "Cannot write wavelet" )
	return false;
    }

    return true;
}

    CtxtIOObj&		ctio_;
    uiGenInput*		isrickfld;
    uiGenInput*		freqfld;
    uiGenInput*		srfld;
    uiGenInput*		peakamplfld;
    uiIOObjSel*		wvltfld;

};


void uiSeisWvltMan::crPush( CallBacker* )
{
    uiSeisWvltManCrWvlt dlg( this );
    if ( !dlg.go() ) return;

    selgrp->fullUpdate( dlg.ctio_.ioobj->key() );
}


void uiSeisWvltMan::mkFileInfo()
{
    BufferString txt;
    Wavelet* wvlt = Wavelet::get( curioobj_ );

    if ( !wvlt )
	fddata_->set( true, 0, "" );
    else
    {
	const int wvltsz = wvlt->size();
	assign( fdctxt_.posdata_.x2rg_, wvlt->samplePositions() );
	delete fda2d_;
	fda2d_ = new Array2DImpl<float>( 1, wvltsz );
	memcpy( fda2d_->getData(), wvlt->samples(), wvltsz * sizeof(float) );
	fddata_->set( true, fda2d_, wvlt->name() );

	Stats::RunCalc<float> rc( Stats::RunCalcSetup().require(Stats::Max) );
	rc.addValues( wvltsz, wvlt->samples() );

	BufferString tmp;
	tmp += "Number of samples: "; tmp += wvlt->size(); tmp += "\n";
	tmp += "Sample interval "; tmp += SI().getZUnit(true); tmp += ": ";
	tmp += wvlt->sampleRate() * SI().zFactor(); tmp += "\n";
	tmp += "Min/Max amplitude: ";
	tmp += rc.min(); tmp += "/"; tmp += rc.max(); tmp += "\n";
	txt += tmp;

	delete wvlt;
    }

    wvltfld->setData( fddata_ );
    wvltfld->forceReDraw();

    txt += getFileInfo();
    infofld->setText( txt );
}
