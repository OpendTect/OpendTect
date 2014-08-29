/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uiwaveletmatchdlg.h"

#include "arrayndimpl.h"
#include "wavelet.h"
#include "waveletattrib.h"

#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseiswvltsel.h"

uiWaveletMatchDlg::uiWaveletMatchDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Match Wavelets"),mNoDlgTitle,mNoHelpKey))
    , wvltid_(MultiID::udf())
{
    wvlt0fld_ = new uiSeisWaveletSel( this, "Source Wavelet", true, false );
    wvlt1fld_ = new uiSeisWaveletSel( this, "Target Wavelet", true, false );
    wvlt1fld_->attach( alignedBelow, wvlt0fld_ );

    IOObjContext ctxt( mIOObjContext(Wavelet) );
    ctxt.forread = false;
    outwvltfld_ = new uiIOObjSel( this, ctxt, "Output Wavelet" );
    outwvltfld_->attach( alignedBelow, wvlt1fld_ );
}


bool uiWaveletMatchDlg::acceptOK( CallBacker* )
{
    Wavelet* srcwvlt = wvlt0fld_->getWavelet();
    if ( !srcwvlt ) return false;

    Wavelet* tarwvlt = wvlt1fld_->getWavelet();
    if ( !tarwvlt ) return false;

    const IOObj* outioobj = outwvltfld_->ioobj();
    if ( !outioobj ) return false;

    const int sz = mMAX(srcwvlt->size(),tarwvlt->size());
    if ( sz != srcwvlt->size() )
	srcwvlt->reSample( mCast(float,sz) );
    else
	tarwvlt->reSample( mCast(float,sz) );

// TODO: Apply window before FFT?
    Array1DImpl<float_complex> fftsrcwvlt( sz );
    WaveletAttrib wvltattr( *srcwvlt );
    wvltattr.transform( fftsrcwvlt, sz );

    Array1DImpl<float_complex> ffttarwvlt( sz );
    wvltattr.setNewWavelet( *tarwvlt );
    wvltattr.transform( ffttarwvlt, sz );

    Array1DImpl<float_complex> fftnewwvlt( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	float_complex val = ffttarwvlt.get(idx) / fftsrcwvlt.get(idx);
	fftnewwvlt.set( idx, val );
    }

    Array1DImpl<float> newwvlt( sz );
    WaveletAttrib::transformBack( fftnewwvlt, newwvlt );

    Wavelet wvlt;
    wvlt.reSize( sz );
    wvlt.setCenterSample( sz/2 );
    for ( int idx=0; idx<sz; idx++ )
	wvlt.set( idx, newwvlt.get(idx) );

    const bool res = wvlt.put( outioobj );
    if ( !res )
    {
	uiMSG().error( "Cannot store new wavelet" );
	return false;
    }

    return true;
}
