/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseiswvltgen.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "iodir.h"
#include "iodirentry.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletio.h"
#include "waveletattrib.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseiswvltsel.h"
#include "uiwindowfuncseldlg.h"
#include "od_helpids.h"


uiSeisWvltCreate::uiSeisWvltCreate( uiParent* p, uiDialog::Setup su )
	: uiDialog(p,su)
{
    wvltfld_ = new uiWaveletSel( this, false );
}


uiSeisWvltCreate::~uiSeisWvltCreate()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSeisWvltCreate::putWvlt( const Wavelet& wvlt )
{
    const IOObj* ioobj = wvltfld_->ioobj( true );
    if ( !ioobj )
	mErrRet( tr("Please enter a name for the new Wavelet") );

    if ( !wvlt.put(ioobj) )
	mErrRet( tr("Cannot write wavelet") )

    return true;
}


MultiID uiSeisWvltCreate::storeKey() const
{
    return wvltfld_->key();
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
    const float deffrq = sCast( float, mNINT32(getFreqScaler()*0.1f/sisr) );
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


static const char* centernms[] = { "Maximum amplitude", "Maximum energy", 0 };
uiSeisWvltMerge::uiSeisWvltMerge( uiParent* p, const char* curwvltnm )
    : uiSeisWvltCreate(p,uiDialog::Setup(tr("Stack Wavelets"),
		       mNoDlgTitle,mODHelpKey(mSeisWvltMergeHelpID)))
    , maxwvltsize_(0)
    , stackedwvlt_(nullptr)
    , curwvltnm_(curwvltnm)
{
    normalizefld_ = new uiCheckBox( this, tr("Normalize wavelets") );
    mAttachCB( normalizefld_->activated, uiSeisWvltMerge::reloadAll );

    centerfld_ = new uiCheckBox( this, tr("Center wavelets") );
    mAttachCB( centerfld_->activated, uiSeisWvltMerge::centerChged );
    mAttachCB( centerfld_->activated, uiSeisWvltMerge::reloadAll );
    centerfld_->attach( rightOf, normalizefld_ );

    centerchoicefld_ = new uiLabeledComboBox( this, centernms, tr("at") );
    mAttachCB( centerchoicefld_->box()->selectionChanged,
	       uiSeisWvltMerge::reloadAll );
    centerchoicefld_->box()->setHSzPol( uiObject::MedVar );
    centerchoicefld_->attach( rightOf, centerfld_ );
    centerchoicefld_->display( false );

    reloadWvlts();
    constructDrawer( false );
    constructDrawer( true );
    reloadFunctions();

    wvltfld_->setLabelText( tr("Save stacked wavelet") );
    wvltfld_->setSensitive( false );

    for ( int idx=0; idx<wvltdrawer_.size(); idx++ )
    {
	wvltdrawer_[idx]->setAsCurrent( curwvltnm );
	wvltdrawer_[idx]->display( !idx );
	mAttachCB( wvltdrawer_[idx]->funclistselChged,
		   uiSeisWvltMerge::funcSelChg );
	normalizefld_->attach( alignedAbove, wvltdrawer_[idx] );
	wvltfld_->attach( ensureBelow, wvltdrawer_[idx] );
    }
}


uiSeisWvltMerge::~uiSeisWvltMerge()
{
    detachAllNotifiers();
    deepErase( wvltset_ );
    deepErase( wvltfuncset_ );
    stackedwvlt_ = nullptr;
}


void uiSeisWvltMerge::funcSelChg( CallBacker* )
{
    uiFuncSelDraw* wd = getCurrentDrawer();
    if ( !wd )
	return;

    NotifyStopper nsf( wd->funclistselChged );
    const int selsz = wd->getNrSel();
    if ( selsz==1 && wd->isSelected( wd->getListSize()-1 ) )
	return;

    clearStackedWvlt( nullptr );
    wvltfld_->setSensitive( selsz > 1 );
    if ( selsz <= 1 ) return;

    makeStackedWvlt();
    wd->funcCheckChg( nullptr );

    TypeSet<int> selitems;
    wd->getSelectedItems( selitems );
    uiFuncSelDraw* od = wvltdrawer_[!normalizefld_->isChecked()];
    od->setSelectedItems( selitems ); // select same items in other drawer
}


void uiSeisWvltMerge::clearStackedWvlt( uiFuncSelDraw* )
{
    if ( !stackedwvlt_ )
	return;

    for ( int idx=0; idx<stackedwvlt_->size(); idx++ )
	stackedwvlt_->samples()[idx] = 0;
}


void uiSeisWvltMerge::makeStackedWvlt()
{
    uiFuncSelDraw* wd = getCurrentDrawer();
    if ( !wd )
	return;

    TypeSet<int> selitems;
    wd->getSelectedItems( selitems );
    int selsize = selitems.size();
    if ( selitems.last() == wvltset_.size()-1 )
	selsize--;

    for ( int selidx=0; selidx<selsize; selidx++ )
    {
	WvltMathFunction* func = wvltfuncset_[selitems[selidx]];
	for ( int idx=0; idx<=maxwvltsize_; idx++ )
	{
	    const float xval = wvltsampling_.atIndex( idx );
	    const float val = func->getValue( xval );
	    stackedwvlt_->samples()[idx] += val/selsize;
	}
    }
}


void uiSeisWvltMerge::constructDrawer( bool isnormalized )
{
    float minhght=0;
    float maxhght=0;
    for ( int wvltidx=0; wvltidx<wvltset_.size(); wvltidx++ )
    {
	Wavelet* wvlt = new Wavelet( *wvltset_[wvltidx] );
	if ( isnormalized ) wvlt->normalize();

	const float minval = wvlt->getExtrValue(false);
	const float maxval = wvlt->getExtrValue(true);
	if ( minval < minhght ) minhght = minval;
	if ( maxval > maxhght ) maxhght = maxval;
	delete wvlt;
    }

    StepInterval<float> xaxrg( wvltsampling_ );
    const StepInterval<float> yaxrg( minhght, maxhght, (maxhght-minhght)/8);

    uiFunctionDrawer::Setup su; su.name_ = "Wavelet Stacking";
    xaxrg = xaxrg.niceInterval( 5 );
    su.funcrg_ = xaxrg;
    xaxrg.scale( float(SI().zDomain().userFactor()) );
    su.xaxrg_ = xaxrg;
    su.yaxrg_ = yaxrg.niceInterval( 5 );

    su.xaxcaption_ = tr("Time %1").arg(SI().getUiZUnitString());
    su.yaxcaption_ = tr("Amplitude");

    wvltdrawer_ += new uiFuncSelDraw( this, su );
}


uiFuncSelDraw* uiSeisWvltMerge::getCurrentDrawer()
{ return wvltdrawer_[normalizefld_->isChecked()]; }


void uiSeisWvltMerge::reloadWvlts()
{
    deepErase( wvltset_ );
    stackedwvlt_ = nullptr;
    namelist_.setEmpty();

    IOObjContext ctxt = mIOObjContext(Wavelet);
    const IODir iodir( ctxt.getSelKey() );
    const IODirEntryList del( iodir, ctxt );
    if ( del.size() < 2 )
    {
	uiMSG().error( tr("Not enough wavelets available") );
	return;
    }

    Interval<float> xrg;
    float minsampling = mUdf(float);
    for ( int delidx=0; delidx<del.size(); delidx++ )
    {
	const IOObj* ioobj = del[delidx]->ioobj_;
	if ( !ioobj )
	    continue;

	Wavelet* wvlt = Wavelet::get( ioobj );
	if ( !wvlt )
	    continue;

	wvltset_ += wvlt;
	namelist_.add( ioobj->name() );

	xrg.include( wvlt->samplePositions() );
	if ( wvlt->sampleRate() < minsampling )
	    minsampling = wvlt->sampleRate();
    }

    wvltsampling_.set( xrg.start, xrg.stop, minsampling );
    maxwvltsize_ = wvltsampling_.nrSteps() + 1;

    for ( int idx=0; idx<wvltset_.size(); idx++ )
    {
	Wavelet* wvlt = wvltset_[idx];

	if ( !mIsEqual(wvltsampling_.step,wvlt->sampleRate(),mDefEps) )
	    wvlt->reSample( wvltsampling_.step );

	if ( normalizefld_->isChecked() )
	    wvlt->normalize();

	if ( centerfld_->isChecked() )
	{
	    if ( centerchoicefld_->box()->currentItem() )
		centerToMaxEnergyPos( *wvlt );
	    else
		centerToMaxAmplPos( *wvlt );
	}
    }

    const char* wvltname = "<Stacked Wavelet>";
    stackedwvlt_ = new Wavelet( wvltname );
    wvltset_ += stackedwvlt_;
    namelist_.add( wvltname );
    stackedwvlt_->reSize( maxwvltsize_ );
    stackedwvlt_->setSampleRate( wvltsampling_.step );
    stackedwvlt_->setCenterSample( wvltsampling_.nearestIndex(0.f) );
    for ( int idx=0; idx<maxwvltsize_; idx++ )
	stackedwvlt_->samples()[idx] = 0;
}


void uiSeisWvltMerge::reloadFunctions()
{
    uiFuncSelDraw* wd = getCurrentDrawer();
    TypeSet<int> selitms;
    wd->getSelectedItems( selitms );

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

	wvltdrawer_[widx]->setSelectedItems( selitms );
	const int firstsel = selitms.isEmpty() ? 0 : selitms.first();
	wvltdrawer_[widx]->setAsCurrent( namelist_.get(firstsel).buf() );
    }
}


void uiSeisWvltMerge::reloadAll( CallBacker* )
{
    NotifyStopper nsf0( wvltdrawer_[0]->funclistselChged );
    NotifyStopper nsf1( wvltdrawer_[1]->funclistselChged );

    reloadWvlts();
    reloadFunctions();

    funcSelChg( nullptr );
    wvltdrawer_[0]->display( !normalizefld_->isChecked() );
    wvltdrawer_[1]->display( normalizefld_->isChecked() );
}


void uiSeisWvltMerge::centerToMaxEnergyPos( Wavelet& wvlt )
{
    WaveletAttrib wvltattr( wvlt );
    Array1DImpl<float> hilb ( wvlt.size() );
    wvltattr.getHilbert( hilb );

    float max = 0;
    int centeridx = 0;
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
	mErrRet( tr("There is no stacked wavelet to be saved") );

// Get samplerate of selected items only and resample stackedwavelet accordingly
    uiFuncSelDraw* wd = getCurrentDrawer();
    TypeSet<int> selitms;
    wd->getSelectedItems( selitms );
    float sr = mUdf(float);
    for ( int idx=0; idx<selitms.size(); idx++ )
    {
	const int itm = selitms[idx];
	PtrMan<IOObj> wvltioobj = Wavelet::getIOObj( namelist_.get(itm) );
	PtrMan<Wavelet> wvlt = Wavelet::get( wvltioobj );
	if ( !wvlt ) continue;

	if ( wvlt->sampleRate() < sr )
	    sr = wvlt->sampleRate();
    }

    Wavelet wvlt( *stackedwvlt_ );
    if ( sr != wvlt.sampleRate() )
	wvlt.reSample( sr );

    return putWvlt( wvlt );
}


uiSeisWvltMerge::WvltMathFunction::WvltMathFunction( const Wavelet* wvlt )
    : samples_(wvlt->samples())
    , samppos_(wvlt->samplePositions())
    , size_(wvlt->size())
{}


uiSeisWvltMerge::WvltMathFunction::~WvltMathFunction()
{}


float uiSeisWvltMerge::WvltMathFunction::getValue( float t ) const
{
    float x = t - samppos_.start;
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
