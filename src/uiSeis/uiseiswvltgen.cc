/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/


#include "uiseiswvltgen.h"

#include "ctxtioobj.h"
#include "ioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletio.h"
#include "waveletattrib.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiwindowfuncseldlg.h"
#include "od_helpids.h"


uiSeisWvltCreate::uiSeisWvltCreate( uiParent* p, uiDialog::Setup su )
	: uiDialog(p,su)
	, ctio_(*mMkCtxtIOObj(Wavelet))
{
    ctio_.ctxt_.forread_ = false;
    wvltfld_ = new uiIOObjSel( this, ctio_ );
}


uiSeisWvltCreate::~uiSeisWvltCreate()
{
    delete ctio_.ioobj_; delete &ctio_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisWvltCreate::putWvlt( const Wavelet& wvlt )
{
    if ( !wvltfld_->commitInput() )
	mErrRet( uiStrings::phrEnter(tr("a name for the new Wavelet")) );

    if ( !wvlt.put(ctio_.ioobj_) )
	mErrRet( tr("Cannot write wavelet") )

    return true;
}


MultiID uiSeisWvltCreate::storeKey() const
{
    return ctio_.ioobj_ ? ctio_.ioobj_->key() : MultiID("");
}


static float getFreqScaler()
{
    return SI().zIsTime() ? 1.f
			  : SI().depthsInFeet() ? 5280.f : 1000.f;
    /*
       /ft are converted to /miles
       /m are converted to /km
   */
}

uiSeisWvltGen::uiSeisWvltGen( uiParent* p )
    : uiSeisWvltCreate(p,uiDialog::Setup(tr("Create Wavelet"),
				 tr("Specify wavelet creation parameters"),
				 mODHelpKey(mSeisWvltManCrWvltHelpID) ))
{
    isrickfld_ = new uiGenInput( this, tr("Wavelet type"),
				BoolInpSpec(true,tr("Ricker"),tr("Sinc")) );

    const float sisr = SI().zStep();
    const float deffrq = mCast( float, mNINT32(getFreqScaler()*0.1f/sisr) );
    // 20% of nyquist frequency

    uiString txt= tr("Central %1 (%2)")
	.arg( SI().zIsTime() ? uiStrings::sFrequency()
			     : uiStrings::sWaveNumber() )
	.arg( SI().zIsTime() ? "Hz" : SI().depthsInFeet() ? "/miles" : "/km" );

    freqfld_ = new uiGenInput( this, txt, FloatInpSpec(deffrq) );
    freqfld_->attach( alignedBelow, isrickfld_ );

    const float usrsr = sisr * SI().zDomain().userFactor();
    txt = tr("Sample interval %1").arg(SI().getUiZUnitString());
    srfld_ = new uiGenInput( this, txt, FloatInpSpec(usrsr) );
    srfld_->attach( alignedBelow, freqfld_ );

    peakamplfld_ = new uiGenInput(this, tr("Peak amplitude"), FloatInpSpec(1));
    peakamplfld_->attach( alignedBelow, srfld_ );

    wvltfld_->attach( alignedBelow, peakamplfld_ );
}


bool uiSeisWvltGen::acceptOK( CallBacker* )
{
    float freq = freqfld_->getFValue();
    float sr = srfld_->getFValue();
    const float peakampl = peakamplfld_->getFValue();

    if ( mIsUdf(sr) || sr <= 0 )
	mErrRet( tr("The sample interval is not valid") )
    else if ( peakampl == 0 )
	mErrRet( tr("The peak amplitude must be non-zero") )
    else if ( mIsUdf(freq) || freq <= 0 )
	mErrRet( tr("The frequency must be positive") )

    freq /= getFreqScaler();
    sr /= SI().zDomain().userFactor();

    Wavelet wvlt( isrickfld_->getBoolValue(), freq, sr, peakampl );
    return putWvlt( wvlt );
}


static const char* centernms[] = { "maximum amplitude", "maximum Energy", 0 };
uiSeisWvltMerge::uiSeisWvltMerge( uiParent* p, const char* curwvltnm )
    : uiSeisWvltCreate(p,uiDialog::Setup(tr("Stack Wavelets"),
		       mNoDlgTitle,mODHelpKey(mSeisWvltMergeHelpID)))
    , maxwvltsize_(0)
    , stackedwvlt_(0)
    , curwvltnm_(curwvltnm)
{
    normalizefld_ = new uiCheckBox( this, tr("Normalize wavelets") );
    normalizefld_->activated.notify( mCB(this,uiSeisWvltMerge,reloadAll) );

    centerfld_ = new uiCheckBox( this, tr("Center wavelets") );
    centerfld_->activated.notify( mCB(this,uiSeisWvltMerge,centerChged) );
    centerfld_->activated.notify( mCB(this,uiSeisWvltMerge,reloadAll) );
    centerfld_->attach( rightOf, normalizefld_ );

    centerchoicefld_ = new uiLabeledComboBox( this, tr("at") );
    centerchoicefld_->box()->addItems( centernms );
    centerchoicefld_->box()->selectionChanged.notify(
					mCB(this,uiSeisWvltMerge,reloadAll) );
    centerchoicefld_->box()->setHSzPol( uiObject::MedVar );
    centerchoicefld_->attach( rightOf, centerfld_ );
    centerchoicefld_->display( false );

    reloadWvlts();
    constructDrawer( false );
    constructDrawer( true );
    reloadFunctions();

    wvltfld_->setLabelText(tr("Save stacked wavelet"));
    wvltfld_->setSensitive( false );

    for ( int idx=0; idx<wvltdrawer_.size(); idx++ )
    {
	wvltdrawer_[idx]->setAsCurrent( curwvltnm );
	wvltdrawer_[idx]->display( !idx );
	wvltdrawer_[idx]->funclistselChged.notify(
				mCB(this,uiSeisWvltMerge,funcSelChg) );
	normalizefld_->attach( alignedAbove, wvltdrawer_[idx] );
	wvltfld_->attach( ensureBelow, wvltdrawer_[idx] );
    }
}


uiSeisWvltMerge::~uiSeisWvltMerge()
{
    for ( int idx=0; idx<wvltdrawer_.size(); idx++ )
	wvltdrawer_[idx]->funclistselChged.remove(
				mCB(this,uiSeisWvltMerge,funcSelChg) );
    deepErase( wvltset_ );
    deepErase( wvltfuncset_ );
    stackedwvlt_ = 0;
}


#define mGetCurDrawer() uiFuncSelDraw* wd = getCurrentDrawer(); if (!wd) return;
void uiSeisWvltMerge::funcSelChg( CallBacker* )
{
    mGetCurDrawer();
    NotifyStopper nsf( wd->funclistselChged );
    const int selsz = wd->getNrSel();
    if ( selsz==1 && wd->isSelected( wd->getListSize()-1 ) )
	return;

    clearStackedWvlt( wd );
    wvltfld_->setSensitive( selsz > 1 );
    if ( selsz <= 1 ) return;

    makeStackedWvlt();
}


void uiSeisWvltMerge::clearStackedWvlt( uiFuncSelDraw* wd )
{
    if ( stackedwvlt_ )
    {
	delete wvltfuncset_.removeSingle( wd->removeLastItem() );
	wvltset_.removeSingle( wvltset_.size()-1 );
	delete stackedwvlt_; stackedwvlt_=0;
    }
}


void uiSeisWvltMerge::makeStackedWvlt()
{
    mGetCurDrawer();
    TypeSet<int> selitems;
    wd->getSelectedItems( selitems );
    const int selsize = selitems.size();

    const char* wvltname = "Stacked Wavelet";
    stackedwvlt_ = new Wavelet( wvltname );
    wvltset_ += stackedwvlt_;
    stackedwvlt_->reSize( maxwvltsize_ );
    for ( int idx=0; idx<maxwvltsize_; idx++ )
	stackedwvlt_->samples()[idx] = 0;

    for ( int selidx=0; selidx<selsize; selidx++ )
    {
	Wavelet* curwvlt = wvltset_[selitems[selidx]];
	stackedwvlt_->setSampleRate( curwvlt->sampleRate() );
	stackedwvlt_->setCenterSample( maxwvltsize_/2 );
	WvltMathFunction* func = wvltfuncset_[selitems[selidx]];
	for ( int idx=0; idx<maxwvltsize_; idx++ )
	{
	    const int shift = maxwvltsize_%2 ? 1 : 0;
	    const float coeff = mCast( float, 2*idx-maxwvltsize_ + shift );
	    const float val = func->getValue( coeff*5*SI().zStep());
	    stackedwvlt_->samples()[idx] += val/selsize;
	}
    }
    WvltMathFunction* stackedfunc = new WvltMathFunction( stackedwvlt_ );
    wvltfuncset_ += stackedfunc;
    wd->addFunction( wvltname, stackedfunc, false );
    wd->setAsCurrent( wvltname );
}


void uiSeisWvltMerge::constructDrawer( bool isnormalized )
{
    float minhght=0; float maxhght=0;
    for ( int wvltidx=0; wvltidx<wvltset_.size(); wvltidx++ )
    {
	Wavelet* wvlt = new Wavelet( *wvltset_[wvltidx] );
	if ( isnormalized ) wvlt->normalize();
	const int wvltsz = wvlt->size();
	const float minval = wvlt->getExtrValue(false);
	const float maxval = wvlt->getExtrValue(true);
	if ( wvltsz > maxwvltsize_ ) maxwvltsize_ = wvltsz;
	if ( minval < minhght ) minhght = minval;
	if ( maxval > maxhght ) maxhght = maxval;
	delete wvlt;
    }
    const float stopx = SI().zStep()*maxwvltsize_*5;
    const float startx = -stopx;
    const StepInterval<float> xaxrg( startx, stopx, ( stopx - startx)/20 );
    const StepInterval<float> yaxrg( minhght, maxhght, (maxhght-minhght)/8);

    uiFunctionDrawer::Setup su; su.name_ = "Wavelet Stacking";
    su.funcrg_ = xaxrg;
    su.xaxrg_ = xaxrg;		 su.yaxrg_ = yaxrg;
    su.xaxcaption_ = tr("time (s)"); su.yaxcaption_ = tr("Amplitude");

    wvltdrawer_ += new uiFuncSelDraw( this, su );
}


uiFuncSelDraw* uiSeisWvltMerge::getCurrentDrawer()
{ return wvltdrawer_[normalizefld_->isChecked()]; }


void uiSeisWvltMerge::reloadWvlts()
{
    deepErase( wvltset_ ); deepErase( namelist_ ); stackedwvlt_ = 0;
    const IODir iodir( ctio_.ctxt_.getSelKey() );
    const IODirEntryList del( iodir, ctio_.ctxt_ );
    if ( del.size() < 2 )
    { uiMSG().error( tr("not enough wavelets available") ); return; }

    for ( int delidx=0; delidx<del.size(); delidx++ )
    {
	const IOObj* ioobj = del[delidx]->ioobj_;
	if ( !ioobj ) continue;
	Wavelet* wvlt = Wavelet::get( ioobj );
	if ( !wvlt ) continue;
	wvltset_ += wvlt;
	if ( normalizefld_->isChecked() )
	    wvlt->normalize();
	if ( centerfld_->isChecked() )
	{
	    if ( centerchoicefld_->box()->currentItem() )
		centerToMaxEnergyPos( *wvlt );
	    else
		centerToMaxAmplPos( *wvlt );
	}
	namelist_.add( ioobj->name() );
    }
}


void uiSeisWvltMerge::reloadFunctions()
{
    for ( int widx=0; widx<wvltdrawer_.size(); widx++ )
    {
	for ( int idx=wvltdrawer_[widx]->getListSize()-1; idx>=0; idx-- )
	    wvltdrawer_[widx]->removeItem( idx );
    }
    deepErase( wvltfuncset_ );
    for ( int widx=0; widx<wvltdrawer_.size(); widx++ )
    {
	for ( int idx=0; idx<wvltset_.size(); idx++ )
	{
	    wvltfuncset_ += new WvltMathFunction( wvltset_[idx] );
	    wvltdrawer_[widx]->addFunction( namelist_[idx]->buf(),
					    wvltfuncset_[idx] );
	}
    }
}


void uiSeisWvltMerge::reloadAll( CallBacker* )
{
    NotifyStopper nsf0( wvltdrawer_[0]->funclistselChged );
    NotifyStopper nsf1( wvltdrawer_[1]->funclistselChged );

    reloadWvlts();
    reloadFunctions();

    wvltdrawer_[normalizefld_->isChecked()]->setAsCurrent( curwvltnm_ );
    funcSelChg( 0 );
    wvltdrawer_[0]->display( !normalizefld_->isChecked() );
    wvltdrawer_[1]->display( normalizefld_->isChecked() );
}


void uiSeisWvltMerge::centerToMaxEnergyPos( Wavelet& wvlt )
{
    WaveletAttrib wvltattr( wvlt );
    Array1DImpl<float> hilb ( wvlt.size() );
    wvltattr.getHilbert( hilb );

    float max = 0;	int centeridx = 0;
    for ( int idx=0; idx<wvlt.size(); idx++ )
    {
	float val = fabs( hilb.get(idx) );
	if ( max < val )
	{
	    max = val;
	    centeridx = idx;
	}
    }
    wvlt.setCenterSample( centeridx );
}


void uiSeisWvltMerge::centerToMaxAmplPos( Wavelet& wvlt )
{
    int centeridx = 0;
    float extrval = wvlt.getExtrValue( true );
    const float minval = wvlt.getExtrValue( false );
    if ( fabs(minval) > fabs(extrval) )
	extrval = minval;

    for ( int idx=0; idx<wvlt.size(); idx++ )
    {
	if ( mIsEqual(wvlt.samples()[idx],extrval,mDefEps) )
	{
	    centeridx = idx;
	    break;
	}
    }
    wvlt.setCenterSample( centeridx );
}


void uiSeisWvltMerge::centerChged( CallBacker* )
{
    centerchoicefld_->display( centerfld_->isChecked() );
}


bool uiSeisWvltMerge::acceptOK( CallBacker* )
{
    if ( !stackedwvlt_ )
	mErrRet( tr("there is no stacked wavelet to be saved") );
    return putWvlt( *stackedwvlt_ );
}


uiSeisWvltMerge::WvltMathFunction::WvltMathFunction( const Wavelet* wvlt )
		: samples_(wvlt->samples())
		, samppos_(wvlt->samplePositions())
		, size_(wvlt->size())
		{}

float uiSeisWvltMerge::WvltMathFunction::getValue( float t ) const
{
    float x = ( t*0.1f - samppos_.start );
    x /= samppos_.step;
    const int x1 = int(x);
    if ( x1 > size_-1 || x1<0 )
	return 0;
    else if ( x1 == size_-1 )
	return samples_[x1];
    const float val1 = samples_[x1];
    const int x2 = x1+1 ;
    const float val2 = samples_[x2];
    const float factor = ( x-x1 )/( x2-x1 );

    if ( x1==0 || x2 == size_-1 )
	return Interpolate::linearReg1D( val1, val2, factor );
    else
    {
	float val0 = samples_[x1-1];
	float val3 = samples_[x2+1];
	Interpolate::PolyReg1D<float> pr;
	pr.set( val0, val1, val2, val3 );
	return pr.apply( factor );
    }
}
