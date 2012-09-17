/*+
_______________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Nov 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uifreqtaper.cc,v 1.12 2012/07/10 13:06:07 cvskris Exp $";

#include "uifreqtaper.h"
#include "uiamplspectrum.h"
#include "uiaxishandler.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uifunctiondisplay.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uislicesel.h"
#include "uislider.h"

#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "ioman.h"
#include "scaler.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seistrc.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"


uiFreqTaperDlg::uiFreqTaperDlg( uiParent* p, const FreqTaperSetup& s )
    : uiDialog( p, uiDialog::Setup("Frequency taper",
		    "Select taper parameters at cut-off frequency","101.3.3"))
    , cs_(new CubeSampling())		
    , posdlg_(0)
    , funcvals_(0)					
    , seisnm_(s.seisnm_)
    , attrnm_(s.attrnm_)
{
    setCtrlStyle( LeaveOnly );
    
    CallBack cbview = mCB(this,uiFreqTaperDlg,previewPushed);
    previewfld_ = new uiPushButton( this, "&Preview spectrum...", cbview, true);

    uiFuncTaperDisp::Setup su;
    su.leftrg_ = s.minfreqrg_;    
    su.rightrg_ = s.maxfreqrg_;
    su.logscale_ = true; 
    su.is2sided_ = true; 
 
    drawer_ = new uiFuncTaperDisp( this, su );
    tapergrp_ = new uiFreqTaperGrp( this, s, drawer_ );
    tapergrp_->attach( ensureBelow, drawer_ );

    uiSeparator* sep = new uiSeparator( this, "Sep" );
    previewfld_->attach( ensureBelow, sep );
    previewfld_->attach( centeredBelow, tapergrp_ );

    sep->attach( stretchedBelow, tapergrp_  );

    postFinalise().notify( mCB(tapergrp_, uiFreqTaperGrp, taperChged ) );
}


uiFreqTaperDlg::~uiFreqTaperDlg()
{
    delete cs_;
    delete funcvals_;
    delete posdlg_;
}


class uiFreqTaperSelLineDlg : public uiDialog
{
public:

uiFreqTaperSelLineDlg( uiParent* p, const SeisIOObjInfo& objinfo )
	: uiDialog(p,uiDialog::Setup("Select line from Data",0,mNoHelpID))
	, linesfld_(0)  
	, objinfo_(objinfo)  
{
    BufferString complbl ("Compute amplitude spectrum on ");
    if ( objinfo_.is2D() )
    {
	BufferStringSet linenames;
	objinfo.getLineNames( linenames );
	complbl += "line:";
	linesfld_ = new uiLabeledComboBox( this, complbl );
	for ( int idx=0; idx<linenames.size(); idx++ )
	    linesfld_->box()->addItem( linenames.get(idx) );
    }
    else
	inlcrlfld_ = new uiGenInput( this, complbl,
			    BoolInpSpec(true,"Inline","Crossline") );
    setOkText("Next >>" );
}

const char* getLineName()
{ return linesfld_ ? linesfld_->box()->text() : 0; }

bool isInl()
{ return inlcrlfld_->getBoolValue(); }

protected:

    const SeisIOObjInfo& 	objinfo_;
    uiLabeledComboBox* 		linesfld_;
    uiGenInput*			inlcrlfld_;
};


#define mErrRet(msg) \
{ uiMSG().error(msg); return; }
void uiFreqTaperDlg::previewPushed(CallBacker*)
{
    SeisIOObjInfo objinfo( seisnm_ );  
    if ( !objinfo.isOK() )
	mErrRet( "Cannot read input data, please make sure you selected a valid data" );

    objinfo.getRanges( *cs_ );

    const bool is2d = objinfo.is2D();
    uiFreqTaperSelLineDlg lineposdlg( this, objinfo );
    if ( lineposdlg.go() )
    {
	delete posdlg_; posdlg_ = 0;
	ZDomain::Info info( ZDomain::SI() );
	uiSliceSel::Type tp = is2d ? uiSliceSel::TwoD
				   : (lineposdlg.isInl() ? uiSliceSel::Inl 
							 : uiSliceSel::Crl);
	CallBack dummycb;
	posdlg_ = new uiSliceSelDlg( this, *cs_, *cs_, dummycb, tp, info );
	posdlg_->grp()->enableApplyButton( false );
	posdlg_->grp()->enableScrollButton( false );
	posdlg_->setModal( true );
    }
    else
	return;

    if ( posdlg_ &&  posdlg_->go() )
    {
	const CubeSampling cs = posdlg_->getCubeSampling();
	SeisTrcReader rdr( objinfo.ioObj() );

	Seis::RangeSelData* sd = new Seis::RangeSelData( cs );
	sd->lineKey() = LineKey( lineposdlg.getLineName(), attrnm_ );
	rdr.setSelData( sd );
	rdr.prepareWork();

	SeisTrcBuf trcset( true );
	SeisBufReader sbfr( rdr, trcset );
	sbfr.execute();

	if ( !trcset.size() )
	    mErrRet( "No data extracted" );

	Array2DImpl<float> arr2d( trcset.size(), trcset.get(0)->size() );

	for ( int trcidx=0; trcidx<trcset.size(); trcidx++ )
	{
	    SeisTrc* trc = trcset.get(trcidx);
	    if ( !trc ) continue; 

	    for ( int idx=0; idx<trc->size(); idx++ )
	    {
		float val = trc->get( idx, 0 );
		if ( mIsUdf(val) )
		    val = 0;
		if ( idx >= trcset.get(0)->size() )
		    break;
		arr2d.set( trcidx, idx, val );
	    }
	}

	uiAmplSpectrum spec( this );
	spec.setData( arr2d  );
	delete funcvals_;
	funcvals_ = new Array1DImpl<float>( 0 ); 
	spec.getSpectrumData( *funcvals_ );
	drawer_->setup().fillbelowy2_ = true;
	drawer_->setFunction( *funcvals_, spec.getPosRange() );
	drawer_->yAxis(true)->setName( "Power (dB)" );
    }
}



static const char* winname = "CosTaper";
#define mGetData() isminactive_ ? td1_ : td2_;
#define mGetDrawerData() isminactive_ ? drawer_->leftTaperData()\
				      : drawer_->rightTaperData();
#define mMaxRg Interval<float>( td2_.refrg_.start+0.05, datasz_ )
#define mMinRg Interval<float>( 0.05, td1_.refrg_.stop )
#define mCheckLimitRanges()\
    td1_.rg_.limitTo( mMinRg ); 	td2_.rg_.limitTo( mMaxRg );\
    td1_.rg_.stop = td1_.refrg_.stop;   td2_.rg_.start = td2_.refrg_.start;
uiFreqTaperGrp::uiFreqTaperGrp( uiParent* p, 
				const FreqTaperSetup& s, 
				uiFuncTaperDisp* d )
    : uiGroup( p, "Frequency taper main group" )
    , drawer_(d)  
    , td1_(d->leftTaperData())	  
    , td2_(d->rightTaperData())	  
    , freqinpfld_(0)  
    , hasmin_(s.hasmin_)			
    , hasmax_(s.hasmax_)
    , isminactive_(s.hasmin_)
    , allfreqssetable_(s.allfreqssetable_)
    , datasz_((int)(0.5/(SI().zStep())))
{
    mCheckLimitRanges();
    setSlopeFromFreq();
    
    const char* tapertxt = "Slope (dB/Octave)";
    varinpfld_ = new uiGenInput( this, "", FloatInpSpec() );
    varinpfld_->setTitleText ( tapertxt );
    varinpfld_->setValue( td1_.paramval_ );
    varinpfld_->valuechanged.notify(mCB( this, uiFreqTaperGrp, slopeChanged ));
    varinpfld_->valuechanged.notify( mCB(this, uiFreqTaperGrp, taperChged) );
    varinpfld_->setElemSzPol( uiObject::Small );

    inffreqfld_ = new uiGenInput( this, "Start/Stop frequency(Hz)",
				    FloatInpSpec().setName("Min frequency") );
    inffreqfld_->valuechanged.notify( mCB( this, uiFreqTaperGrp, freqChanged));
    inffreqfld_->attach( rightOf, varinpfld_ );
    inffreqfld_->setElemSzPol( uiObject::Small );
				    
    sliderfld_ = new uiSliderExtra( this, uiSliderExtra::Setup("")
				    .withedit(false)
				    .sldrsize(180)
				    .isvertical(false),
				    "slider" );
    sliderfld_->sldr()->setInterval( hasmin_ ? mMinRg : mMaxRg );
    sliderfld_->attach( rightOf, inffreqfld_ );
    sliderfld_->sldr()->valueChanged.notify( 
	    			mCB( this, uiFreqTaperGrp, sliderChanged ) );
    
    supfreqfld_ = new uiGenInput( this, "",
				    FloatInpSpec().setName("Max frequency") );
    supfreqfld_->valuechanged.notify( mCB( this, uiFreqTaperGrp, freqChanged));

    supfreqfld_->attach( rightOf, sliderfld_ );
    supfreqfld_->setElemSzPol( uiObject::Small );

    if ( hasmin_ && hasmax_ )
    {
	freqinpfld_ = new uiGenInput( this, "View ", BoolInpSpec(true, 
					"Min frequency", "Max frequency") );
	freqinpfld_->valuechanged.notify( 
			mCB(this,uiFreqTaperGrp,freqChoiceChged) );
	freqinpfld_->attach( centeredBelow, inffreqfld_ );
    }
    
    setPercentsFromFreq();
    postFinalise().notify( mCB( this, uiFreqTaperGrp, putToScreen ) );
}


void uiFreqTaperGrp::freqChanged( CallBacker* )
{
    TaperData& td = mGetData();
    TaperData& drawerdata = mGetDrawerData();
    Interval<float> newrg( inffreqfld_->getfValue(), supfreqfld_->getfValue());

    td.rg_ = newrg;
    td.refrg_ = newrg;

    sliderfld_->sldr()->setInterval( isminactive_ ? mMinRg : mMaxRg );
    
    drawerdata.rg_ = newrg;
    drawerdata.refrg_ = newrg;
    drawer_->adaptFreqRangesToDataSize( isminactive_, !isminactive_ );

    mCheckLimitRanges();
    setPercentsFromFreq();
    setSlopeFromFreq();
    
    taperChged(0);
}


#define mStopFreqNotifiers()\
    NotifyStopper nsf1( inffreqfld_->valuechanged );\
    NotifyStopper nsf2( supfreqfld_->valuechanged );
void uiFreqTaperGrp::sliderChanged( CallBacker* )
{
    mStopFreqNotifiers()

    float sval = sliderfld_->sldr()->getValue();
    if ( isminactive_ )
	inffreqfld_->setValue( sval );
    else
	supfreqfld_->setValue( sval );

    freqChanged(0);
}


void uiFreqTaperGrp::slopeChanged( CallBacker* )
{
    TaperData& td = mGetData();
    td.slope_ = varinpfld_->getfValue();
    setFreqFromSlope( td.slope_ );
    setPercentsFromFreq();
    taperChged(0);
}


void uiFreqTaperGrp::taperChged( CallBacker* cb )
{
    drawer_->taperChged(0);
    putToScreen(0);
}



#define setToNearestInt(val)\
{\
    int ifr = mNINT32( val  );\
    if ( mIsZero(val-ifr,1e-2) )\
	val = ifr;\
}
#define setTo1Decimal(val)\
{\
    val*=10;\
    val = (int)val;\
    val = (float)val/10;\
}
void uiFreqTaperGrp::putToScreen( CallBacker* )
{
    mStopFreqNotifiers()
    NotifyStopper nsf3( varinpfld_->valuechanged );
    NotifyStopper nsf4( sliderfld_->sldr()->valueChanged );

    TaperData& td = mGetData();

    float freq1 = td.rg_.start;
    float freq2 = td.rg_.stop;

    setTo1Decimal( freq1 );
    setToNearestInt( freq1 ); 
    setTo1Decimal( freq2 );
    setToNearestInt( freq2 );
    
    inffreqfld_->setValue( freq1 );
    supfreqfld_->setValue( freq2 );

    sliderfld_->sldr()->setValue( isminactive_ ? freq1 : freq2 );

    float slope = td.slope_;
    setTo1Decimal( slope );
    setToNearestInt( slope ); 
    varinpfld_->setValue( slope );
    
    if ( !allfreqssetable_ )
    {
	inffreqfld_->setSensitive( hasmin_ && isminactive_ );
	supfreqfld_->setSensitive( hasmax_ && !isminactive_ );
    }
} 


void uiFreqTaperGrp::setPercentsFromFreq()
{
    mStopFreqNotifiers()
    td1_.paramval_ = hasmin_ ? td1_.rg_.start / td1_.rg_.stop : 0;
    td2_.paramval_ = hasmax_ ? ( td2_.rg_.stop - td2_.rg_.start )
			       / ( datasz_ - td2_.rg_.start ) : 0;

    drawer_->setWindows( td1_.paramval_, td2_.paramval_ );
}


#define mDec2Oct 0.301029996 //log(2)
void uiFreqTaperGrp::setFreqFromSlope( float slope )
{
    mStopFreqNotifiers()
    const float slopeindecade = (float)(slope/mDec2Oct);
    const float slopeinhertz = pow( 10, 1./slopeindecade );
    TaperData& td = mGetData();

    if ( isminactive_ )
	td1_.rg_.start = td.rg_.stop/slopeinhertz;
    else
	td2_.rg_.stop = td.rg_.start*slopeinhertz;

    mCheckLimitRanges();
}


void uiFreqTaperGrp::setSlopeFromFreq()
{
    TaperData& d = mGetData();
    float slope = fabs( 1./Math::Log10( d.rg_.stop / d.rg_.start ) );
    d.slope_ = slope*mDec2Oct;
}


void uiFreqTaperGrp::freqChoiceChged( CallBacker* )
{
    if ( freqinpfld_ ) 
	isminactive_ = freqinpfld_->getBoolValue();
    else
	isminactive_ = hasmin_;

    NotifyStopper ns( sliderfld_->sldr()->valueChanged );
    sliderfld_->sldr()->setInterval( isminactive_ ? mMinRg : mMaxRg );

    setSlopeFromFreq();
    putToScreen(0);
}


void uiFreqTaperGrp::setFreqRange( Interval<float> fqrg )
{ 
    td1_.rg_.start = fqrg.start;
    td2_.rg_.stop = fqrg.stop;
    mCheckLimitRanges()
}


Interval<float> uiFreqTaperGrp::getFreqRange() const
{
    return Interval<float> ( td1_.rg_.start, td2_.rg_.stop );
} 




uiFuncTaperDisp::uiFuncTaperDisp( uiParent* p, const Setup& s )
    : uiFunctionDisplay( p, s )
    , is2sided_(s.is2sided_)  
    , window_(0)				
    , funcvals_(0)
    , orgfuncvals_(0)
    , taperChanged(this)    		 
{
    datasz_ = s.datasz_;
    orgdatasz_ = s.datasz_;
    logscale_ = s.logscale_; 
    leftd_.rg_ = s.leftrg_;   	leftd_.refrg_ = s.leftrg_;   
    rightd_.rg_ = s.rightrg_; 	rightd_.refrg_ = s.rightrg_;
    setWindows( 0, 0 );

    xAxis()->setName( s.xaxnm_ ); 	
    yAxis(false)->setName( s.yaxnm_ );
}


uiFuncTaperDisp::~uiFuncTaperDisp()
{
    delete window_;
    delete leftd_.window_; 
    delete rightd_.window_;
    delete orgfuncvals_; 
}
 

void uiFuncTaperDisp::adaptFreqRangesToDataSize( bool isleft, bool isright )
{
    LinScaler scaler( 0, 0, orgdatasz_, datasz_ );
    if ( isleft )
    {
	leftd_.rg_.stop = scaler.scale( leftd_.rg_.stop );
	leftd_.refrg_.stop = leftd_.rg_.stop; 
    }

    if ( isright )
    {
	rightd_.rg_.start = scaler.scale( rightd_.rg_.start );
	rightd_.refrg_.start = rightd_.rg_.start;
    }
}


void uiFuncTaperDisp::setFunction( Array1DImpl<float>& data, Interval<float> rg)
{
    datasz_ = data.info().getSize(0);
    adaptFreqRangesToDataSize( true, true );

    delete orgfuncvals_;
    orgfuncvals_ = new Array1DImpl<float>( data );

    funcvals_ = &data;
    funcdisprg_ = rg;

    setWindows( leftd_.paramval_, rightd_.paramval_ );
}



void uiFuncTaperDisp::setWindows( float leftvar, float rightvar )
{
    if ( leftvar<0 || leftvar>1 )
	leftvar = 0 ;
    if ( rightvar<0 || rightvar>1 )
	rightvar = 0;
    if ( leftvar == 1 ) leftvar -= 0.01;

    if ( is2sided_ )
    {
	delete leftd_.window_; leftd_.window_ = 0;
	leftd_.paramval_ = leftvar;
	leftd_.winsz_ = 2*(int)leftd_.rg_.stop;
	if ( leftvar && leftd_.winsz_ >= 0 )	
	    leftd_.window_ = new ArrayNDWindow( Array1DInfoImpl(leftd_.winsz_), 
						false, winname, leftvar );

	rightd_.paramval_ = rightvar;
	rightd_.winsz_ = 2*( datasz_ - (int)rightd_.rg_.start );
	delete rightd_.window_; rightd_.window_ = 0;
	if ( rightvar && rightd_.winsz_>= 0 )	
	    rightd_.window_ = new ArrayNDWindow(Array1DInfoImpl(rightd_.winsz_),
						false, winname, 1-rightvar );
    }
    delete window_; window_ =0 ;
    window_ = new ArrayNDWindow(Array1DInfoImpl(datasz_),false,winname,leftvar);

    taperChged(0);
}


void uiFuncTaperDisp::taperChged( CallBacker* cb )
{
    if ( !window_ ) return;

    TypeSet<float> xvals;
    if ( is2sided_ )
    {
	for ( int idx=0; idx<datasz_; idx++ )
	{
	    float val = 1;
	    if ( leftd_.window_ && idx < (int)leftd_.rg_.stop )
		val = 1-leftd_.window_->getValues()[leftd_.winsz_/2+idx];

	    if ( rightd_.window_ && idx > (int)rightd_.rg_.start ) 
		val= 1-rightd_.window_->getValues()[idx-(int)rightd_.rg_.start];

	    window_->setValue( idx,  val );
	    
	    xvals += idx;
	}
    }

    if ( funcvals_ )
    {
	window_->apply( orgfuncvals_, funcvals_ );

	if ( logscale_ )
	{
	    for ( int idx=0; idx<datasz_; idx++ )
	    {
		float val = ( 20*Math::Log10( funcvals_->get(idx)+1 ) );
		funcvals_->set( idx, val );
	    }
	}

	setVals( funcdisprg_, window_->getValues(), datasz_ );
	setY2Vals( funcdisprg_, funcvals_->getData(), datasz_ );
    }
    else if ( xvals.size() ) 
	setVals( xvals.arr(), window_->getValues(), datasz_ );

    taperChanged.trigger();
}





uiFreqTaperSel::uiFreqTaperSel( uiParent* p, const Setup& s, 
				const FreqTaperSetup& fsu )
	    : uiWindowFunctionSel(p,s)
	    , freqsetup_(FreqTaperSetup(fsu))  
	    , freqtaperdlg_(0)
{}


void uiFreqTaperSel::winfuncseldlgCB( CallBacker* )
{
    setSelFreqs(0);

    delete freqtaperdlg_;
    freqtaperdlg_ = new uiFreqTaperDlg( this, freqsetup_ );
    freqtaperdlg_->windowClosed.notify( mCB(this,uiFreqTaperSel,windowClosed));
    freqtaperdlg_->show();
}


void uiFreqTaperSel::windowClosed( CallBacker* )
{
    const int winidx = windowtypefld_->getIntValue( 0 )-1;
    if ( !windowfuncs_.validIdx(winidx) && !onlytaper_ ) return;

    varinpfld_->setValue( freqtaperdlg_->getFreqRange() );
    windowChangedCB(0);
}


void uiFreqTaperSel::setIsMinMaxFreq(bool min, bool max)
{
    freqsetup_.hasmin_ = min; freqsetup_.hasmax_ = max;
    varinpfld_->setSensitive( min, 0, 0 );
    varinpfld_->setSensitive( max, 0 ,1 );
}


Interval<float> uiFreqTaperSel::freqValues() const
{
    return varinpfld_->getFInterval();
}


void uiFreqTaperSel::setInputFreqValue( float val, int fldnr )
{
    varinpfld_->setValue( val, fldnr );
    windowChangedCB(0);
}


void uiFreqTaperSel::setRefFreqs( Interval<float> fint )
{
    freqsetup_.minfreqrg_.stop = fint.start;
    freqsetup_.maxfreqrg_.start = fint.stop;
}


void uiFreqTaperSel::setSelFreqs( CallBacker* )
{
    freqsetup_.minfreqrg_.start = freqValues().start;
    freqsetup_.maxfreqrg_.stop = freqValues().stop;
}

