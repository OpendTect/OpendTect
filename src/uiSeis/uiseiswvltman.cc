/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiseiswvltman.cc,v 1.16 2007-02-22 18:14:40 cvskris Exp $
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
#include "uiflatviewer.h"
#include "uigeninput.h"
#include "uimsg.h"


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Wavelet management",
                                     "Manage wavelets",
                                     "103.3.0").nrstatusflds(1),
	    	   WaveletTranslatorGroup::ioContext() )
    , fda2d_(0)
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

    wvltfld = new uiFlatViewer( this );
    FlatDisp::Context& ctxt = wvltfld->context();
    ctxt.annot_.x1_.name_ = "Amplitude";
    ctxt.annot_.x2_.name_ = SI().zIsTime() ? "Time" : "Depth";
    ctxt.ddpars_.dispvd_ = false; ctxt.ddpars_.dispwva_ = true;
    ctxt.ddpars_.wva_.mid_= Color( 150, 150, 100 );
    ctxt.ddpars_.wva_.overlap_ = -0.01; ctxt.ddpars_.wva_.clipperc_ = 0;
    wvltfld->useStoredDefaults( "Wavelet" );

    ctxt.wvaposdata_.setRange( true, StepInterval<double>(0,0,1) );
    wvltfld->setPrefWidth( 60 );
    wvltfld->attach( ensureRightOf, selgrp );
    wvltfld->setStretch( 1, 2 );

    infofld->attach( ensureBelow, butgrp );
    infofld->attach( ensureBelow, wvltfld );
    selgrp->setPrefWidthInChar( 50 );
    infofld->setPrefWidthInChar( 60 );
    selChg(0);
}


uiSeisWvltMan::~uiSeisWvltMan()
{
    delete fda2d_;
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
				 "Specify wavelet creation parameters",
				 "103.3.2"))
    , ctio_(*mMkCtxtIOObj(Wavelet))
{
    isrickfld = new uiGenInput( this, "Wavelet type",
				BoolInpSpec(true,"Ricker","Sinc") );

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

    FlatDisp::Data& fddata = wvltfld->data();
    if ( !wvlt )
	fddata.set( true, 0, "" );
    else
    {
	const int wvltsz = wvlt->size();
	const float zfac = SI().zFactor();

	delete fda2d_;
	fda2d_ = new Array2DImpl<float>( 1, wvltsz );
	memcpy( fda2d_->getData(), wvlt->samples(), wvltsz * sizeof(float) );
	fddata.set( true, fda2d_, wvlt->name() );
	StepInterval<double> posns(
		StepInterval<double>.setFrom(wvlt->samplePositions()));
	if ( SI().zIsTime() ) posns.scale( zfac );
	wvltfld->context().wvaposdata_.setRange( false, posns );

	Stats::RunCalc<float> rc( Stats::RunCalcSetup().require(Stats::Max) );
	rc.addValues( wvltsz, wvlt->samples() );

	BufferString tmp;
	tmp += "Number of samples: "; tmp += wvlt->size(); tmp += "\n";
	tmp += "Sample interval "; tmp += SI().getZUnit(true); tmp += ": ";
	tmp += wvlt->sampleRate() * zfac; tmp += "\n";
	tmp += "Min/Max amplitude: ";
	tmp += rc.min(); tmp += "/"; tmp += rc.max(); tmp += "\n";
	txt += tmp;

	delete wvlt;
    }

    wvltfld->handleChange( FlatDisp::Viewer::All );

    txt += getFileInfo();
    infofld->setText( txt );
}
