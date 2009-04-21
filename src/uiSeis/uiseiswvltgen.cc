/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltgen.cc,v 1.1 2009-04-21 13:55:59 cvsbruno Exp $";


#include "uiseiswvltgen.h"
#include "wavelet.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "survinfo.h"

#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uimsg.h"


uiSeisWvltGen::uiSeisWvltGen( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Create Wavelet",
				 "Specify wavelet creation parameters",
				 "103.3.2"))
    , ctio_(*mMkCtxtIOObj(Wavelet))
{
    isrickfld_ = new uiGenInput( this, "Wavelet type",
				BoolInpSpec(true,"Ricker","Sinc") );

    const float sisr = SI().zStep();
    float deffrq = 0.1 / sisr; int ideffr = mNINT(deffrq);
    if ( ideffr > 0 && mIsZero(deffrq-ideffr,1e-4) )
	deffrq = ideffr; // avoid awkward 99.999 display
    BufferString txt( "Central " );
    txt += SI().zIsTime() ? "Frequency" : "Wavenumber";
    freqfld_ = new uiGenInput( this, txt, FloatInpSpec(deffrq) );
    freqfld_->attach( alignedBelow, isrickfld_ );

    const float usrsr = sisr * SI().zFactor();
    txt = "Sample interval "; txt += SI().getZUnitString();
    srfld_ = new uiGenInput( this, txt, FloatInpSpec(usrsr) );
    srfld_->attach( alignedBelow, freqfld_ );

    peakamplfld_ = new uiGenInput( this, "Peak amplitude", FloatInpSpec(1) );
    peakamplfld_->attach( alignedBelow, srfld_ );

    ctio_.ctxt.forread = false;
    wvltfld_ = new uiIOObjSel( this, ctio_ );
    wvltfld_->attach( alignedBelow, peakamplfld_ );
}


uiSeisWvltGen::~uiSeisWvltGen()
{
    delete ctio_.ioobj; delete &ctio_;
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisWvltGen::acceptOK( CallBacker* )
{
    if ( !wvltfld_->commitInput() )
	mErrRet( "Please enter a name for the new Wavelet" );

    const float sr = srfld_->getfValue();
    const float peakampl = peakamplfld_->getfValue();
    const float freq = freqfld_->getfValue();

    if ( mIsUdf(sr) || sr <= 0 )
	mErrRet( "The sample interval is not valid" )
    else if ( peakampl == 0 )
	mErrRet( "The peak amplitude must be non-zero" )
    else if ( mIsUdf(freq) || freq <= 0 )
	mErrRet( "The frequency must be positive" )

    const float realsr = sr / SI().zFactor();
    Wavelet wvlt( isrickfld_->getBoolValue(), freq, realsr, peakampl );
    if ( !wvlt.put(ctio_.ioobj) )
	mErrRet( "Cannot write wavelet" )

    return true;
}


MultiID uiSeisWvltGen::storeKey() const
{
    return ctio_.ioobj ? ctio_.ioobj->key() : MultiID("");
}
