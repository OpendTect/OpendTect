/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Sept 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltattr.cc,v 1.3 2009-09-09 13:55:51 cvsbruno Exp $";


#include "uibutton.h"
#include "uiseiswvltattr.h"
#include "uislider.h"

#include "arrayndimpl.h"
#include "wavelet.h"

uiSeisWvltRotDlg::uiSeisWvltRotDlg( uiParent* p, Wavelet* wvlt )
    : uiDialog(p,uiDialog::Setup("Phase Rotation","",mTODOHelpID))
    , wvlt_(wvlt)
    , orgwvlt_(new Wavelet(*wvlt))
    , sliderfld_(0) 
    , phaserotating(this)		    
{
    hilbert_ = new HilbertTransform();
    sliderfld_ = new uiSliderExtra( this, uiSliderExtra::Setup(
				    "Rotation (degree)")
				    .withedit(true)
				    .sldrsize(250)
				    .isvertical(true),
				    "Phase Rotation slider" );
    StepInterval<float> sintv( -180.0, 180.0, 1 );
    sliderfld_->sldr()->setInterval( sintv );
    sliderfld_->sldr()->valueChanged.notify( 
	    		mCB(this,uiSeisWvltRotDlg,sliderMove) );
    sliderfld_->attach( hCentered );
}     



uiSeisWvltRotDlg::~uiSeisWvltRotDlg()
{
    delete orgwvlt_;
    delete hilbert_;
}


void uiSeisWvltRotDlg::sliderMove( CallBacker* )
{
    const float slval = sliderfld_->sldr()->getValue();
    rotatePhase( slval );
}


void uiSeisWvltRotDlg::rotatePhase( float dphase )
{
    const int wvltsz = wvlt_->size();
    Array1DImpl<float> hilsamps ( wvltsz ), samps ( wvltsz  );
    float* wvltsamps = wvlt_->samples();
    memcpy( samps.getData(), orgwvlt_->samples(), wvltsz*sizeof(float) );

    hilbert_->setInputInfo(Array1DInfoImpl( wvltsz ));
    hilbert_->setCalcRange( 0, wvltsz, 0 );
    hilbert_->setDir( true );
    hilbert_->init();
    hilbert_->transform( samps, hilsamps );

    for ( int idx=0; idx<wvltsz; idx++ )
	wvltsamps[idx] = samps.get(idx)*cos( dphase*M_PI/180 ) 
			- hilsamps.get(idx)*sin( dphase*M_PI/180 );
    phaserotating.trigger();
}



