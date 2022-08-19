/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwaveletmatchdlg.h"

#include "arrayndimpl.h"
#include "genericnumer.h"
#include "linsolv.h"
#include "wavelet.h"
#include "waveletattrib.h"

#include "uiaxishandler.h"
#include "uifuncdispbase.h"
#include "uifunctiondisplayserver.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseiswvltsel.h"


static const int sDispWidth = 150;
static const int sDispHeight = 150;


uiWaveletMatchDlg::uiWaveletMatchDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Match Wavelets"),mNoDlgTitle,mNoHelpKey))
    , wvltid_(MultiID::udf())
    , outputwvlt_(*new Wavelet)
{
    uiFuncDispBase::Setup fds;
    fds.canvasheight(sDispHeight).canvaswidth(sDispWidth).noy2axis(true).
	noy2gridline(true);
    wvlt0disp_ = GetFunctionDisplayServer().createFunctionDisplay( this, fds );
    wvlt0disp_->setTitle( tr("Reference Wavelet") );

    wvlt1disp_ = GetFunctionDisplayServer().createFunctionDisplay( this, fds );
    wvlt1disp_->setTitle( tr("Target Wavelet") );
    wvlt1disp_->uiobj()->attach( rightTo, wvlt0disp_->uiobj() );
    wvlt1disp_->uiobj()->attach( heightSameAs, wvlt0disp_->uiobj() );

    wvltoutdisp_ = GetFunctionDisplayServer().createFunctionDisplay(this, fds);
    wvltoutdisp_->setTitle( uiStrings::phrOutput( uiStrings::sWavelet() ) );
    wvltoutdisp_->uiobj()->attach( alignedBelow, wvlt0disp_->uiobj() );
    wvltoutdisp_->uiobj()->attach( heightSameAs, wvlt1disp_->uiobj() );

    fds.useyscalefory2(true);
    wvltqcdisp_ = GetFunctionDisplayServer().createFunctionDisplay( this, fds );
    wvltqcdisp_->setTitle( tr("QC") );
    wvltqcdisp_->uiobj()->attach( rightTo, wvltoutdisp_->uiobj() );
    wvltqcdisp_->uiobj()->attach( heightSameAs, wvlt1disp_->uiobj() );

    uiIOObjSel::Setup setup0( tr("Reference Wavelet") );
    setup0.filldef(false).withclear(true);
    wvlt0fld_ = new uiWaveletSel( this, true, setup0 );
    wvlt0fld_->attach( alignedBelow, wvltoutdisp_->uiobj() );
    wvlt0fld_->setStretch( 0, 0 );
    wvlt0fld_->selectionDone.notify( mCB(this,uiWaveletMatchDlg,inpSelCB) );

    uiIOObjSel::Setup setup1( tr("Target Wavelet") );
    setup1.filldef(false).withclear(true);
    wvlt1fld_ = new uiWaveletSel( this, true, setup1 );
    wvlt1fld_->attach( alignedBelow,  wvlt0fld_ );
    wvlt1fld_->setStretch( 0, 0 );
    wvlt1fld_->selectionDone.notify( mCB(this,uiWaveletMatchDlg,inpSelCB) );

    IntInpSpec filterrg( 21 ); filterrg.setLimits( StepInterval<int>(3,99,2) );
    filterszfld_ = new uiGenInput( this, tr("Filter length"), filterrg );
    filterszfld_->valuechanging.notify( mCB(this,uiWaveletMatchDlg,filterSzCB));
    filterszfld_->attach( rightOf, wvlt1fld_ );

    wvltoutfld_ = new uiWaveletSel( this, false );
    wvltoutfld_->attach( alignedBelow, wvlt1fld_ );
    wvltoutfld_->setStretch( 0, 0 );
}


uiWaveletMatchDlg::~uiWaveletMatchDlg()
{
    delete &outputwvlt_;
}


void uiWaveletMatchDlg::inpSelCB( CallBacker* cb )
{
    mDynamicCastGet(uiWaveletSel*,inpfld,cb)
    if ( !inpfld ) return;

    PtrMan<Wavelet> wvlt = inpfld->getWavelet( true );
    if ( !wvlt ) return;

    uiFuncDispBase* fd = inpfld==wvlt0fld_ ? wvlt0disp_ : wvlt1disp_;
    fd->setVals( wvlt->samplePositions(), wvlt->samples(), wvlt->size() );

    calcFilter();
}


void uiWaveletMatchDlg::filterSzCB( CallBacker* )
{
    calcFilter();
}


static void solveAxb( int sz, float* a, float* b, float* x )
{
    Array2DImpl<float> A( sz, sz );
    A.setAll( 0 );
    for ( int idx=0; idx<sz; idx++ )
    {
	for ( int idy=0; idy<sz; idy++ )
	{
	    const int vecidx = Math::Abs(idy-idx) + sz/2;
	    if ( vecidx<sz )
		A.set( idx, idy, a[vecidx] );
	}
    }

    LinSolver<float> ls( A );
    if ( ls.init() )
	ls.apply( b, x );
}


bool uiWaveletMatchDlg::calcFilter()
{
    PtrMan<Wavelet> refwvlt = wvlt0fld_->getWavelet( true );
    PtrMan<Wavelet> tarwvlt = wvlt1fld_->getWavelet( true );
    if ( !refwvlt || !tarwvlt ) return false;

    const int filtersz = filterszfld_->getIntValue();
    const int filtersz2 = mCast(int,filtersz/2);
    mAllocVarLenArr(float,autoref,filtersz);
    mAllocVarLenArr(float,crossref,filtersz);

    const float* wref = refwvlt->samples();
    const int refsz = refwvlt->size();
    float* autorefptr = mVarLenArr( autoref );
    genericCrossCorrelation( refsz, 0, wref,
			     refsz, 0, wref,
			     filtersz, -filtersz2, autorefptr );

    const float* wtar = tarwvlt->samples();
    const int tarsz = tarwvlt->size();
    float* crossrefptr = mVarLenArr( crossref );
    genericCrossCorrelation( refsz, 0, wref,
			     tarsz, 0, wtar,
			     filtersz, -filtersz2, crossrefptr );

//  Solve Ax=b
    outputwvlt_.reSize( filtersz );
    outputwvlt_.setCenterSample( filtersz2 );
    float* x = outputwvlt_.samples();
    solveAxb( filtersz, autoref, crossref, x );

    Interval<float> intv( -(float)filtersz2, (float)filtersz2 );
    wvltoutdisp_->setVals( intv, x, filtersz );

    wvltqcdisp_->setVals( tarwvlt->samplePositions(), wtar, tarsz );
//  QC: Convolve result with reference wavelet
    mAllocVarLenArr(float,wqc,tarsz)
    float* wqcptr = mVarLenArr( wqc );
    GenericConvolve( refsz,0,wref, filtersz,-filtersz/2,x, tarsz,0,wqcptr );
    wvltqcdisp_->setY2Vals( tarwvlt->samplePositions(), wqc, tarsz );
    wvltqcdisp_->draw();

//  Calculate similarity between QC and target wavelet
    const float simi = similarity( wtar, wqcptr, tarsz, true );
    uiString title( tr("QC - Similarity: %1") );
    title.arg( simi );
    wvltqcdisp_->setTitle( title );

    return true;
}


bool uiWaveletMatchDlg::acceptOK( CallBacker* )
{
    const IOObj* ioobj = wvltoutfld_->ioobj();
    if ( !ioobj ) return false;

    const bool res = outputwvlt_.put( ioobj );
    if ( !res )
    {
	uiMSG().error( tr("Cannot store new wavelet") );
	return false;
    }

    return true;
}
