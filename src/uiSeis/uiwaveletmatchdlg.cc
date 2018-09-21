/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2014
________________________________________________________________________

-*/


#include "uiwaveletmatchdlg.h"

#include "genericnumer.h"
#include "wavelet.h"
#include "waveletattrib.h"

#include "uiaxishandler.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiusershowwait.h"
#include "uiwaveletsel.h"


static const int sDispWidth = 150;
static const int sDispHeight = 150;


static void initFunctionDisplay( uiFunctionDisplay& fd )
{
    fd.enableScrollZoom();
    fd.setScrollBarPolicy( true, uiGraphicsViewBase::ScrollBarAlwaysOff );
    fd.setScrollBarPolicy( false, uiGraphicsViewBase::ScrollBarAlwaysOff );
    fd.setDragMode( uiGraphicsViewBase::ScrollHandDrag );
}


uiWaveletMatchDlg::uiWaveletMatchDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Match Wavelets"),mNoDlgTitle,mNoHelpKey))
    , outputwvlt_(*new Wavelet)
{
    uiFunctionDisplay::Setup fds;
    fds.canvasheight(sDispHeight).canvaswidth(sDispWidth);
    wvlt0disp_ = new uiFunctionDisplay( this, fds );
    wvlt0disp_->setTitle( tr("Reference Wavelet") );
    initFunctionDisplay( *wvlt0disp_ );

    wvlt1disp_ = new uiFunctionDisplay( this, fds );
    wvlt1disp_->setTitle( tr("Target Wavelet") );
    wvlt1disp_->attach( rightTo, wvlt0disp_ );
    wvlt1disp_->attach( heightSameAs, wvlt0disp_ );
    initFunctionDisplay( *wvlt1disp_ );

    wvltoutdisp_ = new uiFunctionDisplay( this, fds );
    wvltoutdisp_->setTitle( uiStrings::phrOutput( uiStrings::sWavelet() ) );
    wvltoutdisp_->attach( alignedBelow, wvlt0disp_ );
    wvltoutdisp_->attach( heightSameAs, wvlt1disp_ );
    initFunctionDisplay( *wvltoutdisp_ );

    wvltqcdisp_ = new uiFunctionDisplay( this, fds );
    wvltqcdisp_->setTitle( tr("QC") );
    wvltqcdisp_->attach( rightTo, wvltoutdisp_ );
    wvltqcdisp_->attach( heightSameAs, wvlt1disp_ );
    initFunctionDisplay( *wvltqcdisp_ );

    uiWaveletIOObjSel::Setup setup0( tr("Reference Wavelet") );
    setup0.filldef(false).withclear(true);
    wvlt0fld_ = new uiWaveletIOObjSel( this, setup0 );
    wvlt0fld_->attach( alignedBelow, wvltoutdisp_ );
    wvlt0fld_->setStretch( 0, 0 );
    wvlt0fld_->selectionDone.notify( mCB(this,uiWaveletMatchDlg,inpSelCB) );

    uiWaveletIOObjSel::Setup setup1( tr("Target Wavelet") );
    setup1.filldef(false).withclear(true);
    wvlt1fld_ = new uiWaveletIOObjSel( this, setup1 );
    wvlt1fld_->attach( alignedBelow,  wvlt0fld_ );
    wvlt1fld_->setStretch( 0, 0 );
    wvlt1fld_->selectionDone.notify( mCB(this,uiWaveletMatchDlg,inpSelCB) );

    IntInpSpec filterrg( 21 ); filterrg.setLimits( StepInterval<int>(3,99,2) );
    filterszfld_ = new uiGenInput( this, tr("Filter length"), filterrg );
    filterszfld_->valuechanging.notify( mCB(this,uiWaveletMatchDlg,filterSzCB));
    filterszfld_->attach( rightOf, wvlt1fld_ );

    wvltoutfld_ = new uiWaveletIOObjSel( this, false );
    wvltoutfld_->attach( alignedBelow, wvlt1fld_ );
    wvltoutfld_->setStretch( 0, 0 );
}


uiWaveletMatchDlg::~uiWaveletMatchDlg()
{
    outputwvlt_.unRef();
}


void uiWaveletMatchDlg::inpSelCB( CallBacker* cb )
{
    mDynamicCastGet(uiWaveletIOObjSel*,inpfld,cb)
    if ( !inpfld )
	return;

    ConstRefMan<Wavelet> wvlt = inpfld->getWavelet();
    if ( !wvlt )
	return;

    uiFunctionDisplay* fd = inpfld==wvlt0fld_ ? wvlt0disp_ : wvlt1disp_;
    TypeSet<float> samps; wvlt->getSamples( samps );
    fd->setVals( wvlt->samplePositions(), samps.arr(), samps.size() );

    calcFilter();
}


void uiWaveletMatchDlg::filterSzCB( CallBacker* )
{
    calcFilter();
}


bool uiWaveletMatchDlg::calcFilter()
{
    ConstRefMan<Wavelet> refwvlt = wvlt0fld_->getWavelet();
    ConstRefMan<Wavelet> tarwvlt = wvlt1fld_->getWavelet();
    if ( !refwvlt || !tarwvlt )
	return false;

    const int filtersz = filterszfld_->getIntValue();
    uiUSWTaskRunnerProvider trprov( this, tr("Calculating filter") );
    ConstRefMan<Wavelet> outwvlt = getMatchFilter( *refwvlt, *tarwvlt, trprov,
						   &filtersz );
    if ( !outwvlt )
	return false;

    outputwvlt_ = *outwvlt;

    TypeSet<float> samps; outputwvlt_.getSamples( samps );
    float* psamps = samps.arr();
    wvltoutdisp_->setVals( outputwvlt_.samplePositions(),
			   psamps, outputwvlt_.size() );

    TypeSet<float> tarsamps; tarwvlt->getSamples( tarsamps );
    const float* wtar = tarsamps.arr();
    const int tarsz = tarsamps.size();
    wvltqcdisp_->setVals( tarwvlt->samplePositions(), wtar, tarsz );

//  QC: Convolve result with reference wavelet
    mAllocVarLenArr(float,wqc,tarsz)
    float* wqcptr = mVarLenArr( wqc );
    TypeSet<float> refsamps; refwvlt->getSamples( refsamps );
    const int refsz = refsamps.size();
    GenericConvolve( refsz,0,refsamps.arr(), filtersz,-filtersz/2,psamps,
		     tarsz,0,wqcptr );
    wvltqcdisp_->setY2Vals( tarwvlt->samplePositions(), wqc, tarsz );

//  Make y2 axis identical to y1
    const uiAxisHandler* y1axis = wvlt1disp_->yAxis(false);
    float annotstart = y1axis->annotStart();
    const StepInterval<float> yrg = y1axis->range();
    wvltqcdisp_->yAxis(true)->setRange( yrg, &annotstart );
    wvltqcdisp_->draw();

//  Calculate similarity between QC and target wavelet
    const float simi = similarity( wtar, wqcptr, tarsz, true );
    uiString title( tr("QC - Similarity: %1") );
    title.arg( simi );
    wvltqcdisp_->setTitle( title );

    return true;
}


bool uiWaveletMatchDlg::acceptOK()
{
    return wvltoutfld_->store( outputwvlt_, true );
}
