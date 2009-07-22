/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uislicesel.cc,v 1.54 2009-07-22 16:01:42 cvsbert Exp $";

#include "uislicesel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uispinbox.h"

#include "survinfo.h"
#include "thread.h"
#include "timer.h"

static const char* sButTxtAdvance = "&Advance >>";
static const char* sButTxtPause = "&Pause";


uiSliceSel::uiSliceSel( uiParent* p, Type type )
    : uiGroup(p,"Slice Selection")
    , inl0fld_(0)
    , updatemutex_(*new Threads::Mutex)
    , applycb_(0)
    , scrolldlg_(0)
    , scrollbut_(0)
    , applybut_(0)
{
    isinl_ = type == Inl;
    iscrl_ = type == Crl;
    istsl_ = type == Tsl;
    isvol_ = type == Vol;
    is2d_ = type == TwoD;

    if ( !is2d_ )
	createInlFld();

    createCrlFld();
    createZFld();

    if ( inl0fld_ )
	mainObject()->setTabOrder( (uiObject*)inl0fld_, (uiObject*)crl0fld_ );
    mainObject()->setTabOrder( (uiObject*)crl0fld_, (uiObject*)z0fld_ );

    if ( !isvol_ && !is2d_ )
    {
	applybut_ = new uiPushButton( this, "&Apply", true );
	applybut_->activated.notify( mCB(this,uiSliceSel,applyPush) );
	mainObject()->setTabOrder( (uiObject*)z0fld_, (uiObject*)applybut_ );
	applybut_->attach( alignedBelow, z0fld_ );
	applybut_->display( false );

	scrollbut_ = new uiPushButton( this, "&Scroll", false );
	scrollbut_->activated.notify( mCB(this,uiSliceSel,scrollPush) );
	scrollbut_->attach( rightOf, isinl_ ? inl0fld_
					    : (iscrl_?crl0fld_:z0fld_));
    }

    setHAlignObj( crl0fld_ );
    updateUI();
}


void uiSliceSel::setApplyCB( const CallBack& acb )
{
    delete applycb_;
    applycb_ = new CallBack( acb );
    if ( applybut_ ) applybut_->display( true );
}


void uiSliceSel::createInlFld()
{
    BufferString label( isinl_ ? "Inline nr" : "Inline range" );
    const char* nm = label;
    inl0fld_ = new uiLabeledSpinBox( this, label, 0,
			BufferString(isinl_ ? "Inl nr" : "Inl Start") );
    inl1fld_ = new uiSpinBox( this, 0, "Inl Stop" );
    inl1fld_->attach( rightTo, inl0fld_ );
    inl1fld_->display( !isinl_ );
}


void uiSliceSel::createCrlFld()
{
    BufferString label = is2d_ ? "Trace range" 
			       : ( iscrl_ ? "Xline nr" : "Xline range" );
    crl0fld_ = new uiLabeledSpinBox( this, label, 0,
	   		 BufferString( iscrl_ ? "Crl nr" : "Crl Start ") );
    crl1fld_ = new uiSpinBox( this, 0, "Crl Stop" );
    crl1fld_->attach( rightTo, crl0fld_ );
    crl1fld_->display( !iscrl_ );
    if ( inl0fld_ ) crl0fld_->attach( alignedBelow, inl0fld_ );
}


void uiSliceSel::createZFld()
{
    BufferString label = SI().zIsTime() ? "Time" : "Depth";
    BufferString nm = "Z";
    if ( !istsl_ )
    {
	label += " range "; label += SI().getZUnitString();
	nm += " Start";
    }
    z0fld_ = new uiLabeledSpinBox( this, label, 0, nm );
    z1fld_ = new uiSpinBox( this, 0, "Z Stop" );
    z1fld_->attach( rightTo, z0fld_ );
    z1fld_->display( !istsl_ );
    z0fld_->attach( alignedBelow, crl0fld_ );
}


void uiSliceSel::setBoxValues( uiSpinBox* box, const StepInterval<int>& intv, 
			       int curval )
{
    box->setMinValue( intv.start );
    box->setMaxValue( intv.stop );
    box->setStep( intv.step, true );
    box->setValue( curval );
}



class uiSliceScroll : public uiDialog
{
public:

uiSliceScroll( uiSliceSel* ss )
	: uiDialog(ss,uiDialog::Setup("Scrolling",getTitle(ss),"0.4.2")
				      .modal(false))
	, slcsel(ss)
	, inauto(false)
	, paused(false)
	, zfact( SI().zFactor() )
{
    setCtrlStyle( LeaveOnly );
    timer = new Timer( "uiSliceScroll timer" );
    timer->tick.notify( mCB(this,uiSliceScroll,timerTick) );

    const CubeSampling& cs = SI().sampling( false );
    const HorSampling& hs = cs.hrg;
    int step = hs.step.inl;
    int maxstep = hs.start.inl - hs.stop.inl;
    if  ( ss->iscrl_ )
    {
	step = hs.step.crl;
	maxstep = hs.start.crl - hs.stop.crl;
    }
    else if ( ss->istsl_ )
    {
	step = mNINT(cs.zrg.step*zfact);
	float zrg = (cs.zrg.stop - cs.zrg.start) * zfact;
	maxstep = mNINT(zrg);
    }
    if ( maxstep < 0 ) maxstep = -maxstep;
    stepfld_ = new uiLabeledSpinBox( this, "Scroll step" );
    stepfld_->box()->setMinValue( -maxstep );
    stepfld_->box()->setMaxValue( maxstep );
    stepfld_->box()->setStep( step );
    stepfld_->box()->setValue( step );

    typfld_ = new uiLabeledComboBox( this, "Control" );
    typfld_->box()->addItem( "Manual" );
    typfld_->box()->addItem( "Auto" );
    typfld_->box()->selectionChanged.notify( mCB(this,uiSliceScroll,typSel) );
    typfld_->attach( alignedBelow, stepfld_ );

    ctrlbut = new uiPushButton( this, sButTxtAdvance, true );
    ctrlbut->activated.notify( mCB(this,uiSliceScroll,butPush) );
    ctrlbut->attach( alignedBelow, typfld_ );
    backbut = new uiPushButton( this, "<< Step &Back", true );
    backbut->activated.notify( mCB(this,uiSliceScroll,butPush) );
    backbut->attach( leftOf, ctrlbut );

    dtfld_ = new uiGenInput( this, "Time between updates (s)", FloatInpSpec(2));
    dtfld_->attach( alignedBelow, ctrlbut );

    finaliseDone.notify( mCB(this,uiSliceScroll,typSel) );
}


~uiSliceScroll()
{
    delete timer; timer = 0;
}


void typSel( CallBacker* )
{
    const bool autoreq = typfld_->box()->currentItem() == 1;
    dtfld_->display( autoreq );
    if ( inauto != autoreq )
    {
	if ( autoreq )
	    startAuto();
	else
	    stopAuto( false );
    }
    ctrlbut->setText( autoreq ? sButTxtPause : sButTxtAdvance );
    backbut->display( !autoreq );
    inauto = autoreq;
}


void butPush( CallBacker* cb )
{
    if ( !inauto )
	doAdvance( cb != ctrlbut );
    else
    {
	/*new*/paused = ctrlbut->text()[1] == 'P';
	ctrlbut->setText( paused ? "&Go" : sButTxtPause );
    }
}


void startAuto()
{
    paused = false;
    doAdvance( false );
    ctrlbut->setText( sButTxtPause );
    setTimer();
}

void stopAuto( bool setmanual )
{
    timer->stop();
    inauto = false;
    if ( setmanual )
    {
	typfld_->box()->setCurrentItem( 0 );
	ctrlbut->setText( sButTxtAdvance );
	backbut->display( true );
    }
}


void doAdvance( bool reversed )
{
    if ( !timer ) return;

    const int step = (reversed ? -1 : 1) * stepfld_->box()->getValue();
    slcsel->readInput();
    if ( slcsel->isinl_ )
    {
	int newval = slcsel->cs_.hrg.start.inl + step;
	if ( !SI().sampling(true).hrg.inlOK(newval) )
	    stopAuto( true );
	else
	    slcsel->inl0fld_->box()->setValue( newval );
    }
    else if ( slcsel->iscrl_ )
    {
	int newval = slcsel->cs_.hrg.start.crl + step;
	if ( !SI().sampling(true).hrg.crlOK(newval) )
	    stopAuto( true );
	else
	    slcsel->crl0fld_->box()->setValue( newval );
    }
    else
    {
	const float zfac = SI().zFactor();
	float newval = slcsel->cs_.zrg.start + step / zfac;
	if ( !SI().sampling(true).zrg.includes(newval) )
	    stopAuto( true );
	else
	{
	    if ( zfac < 10 )
		slcsel->z0fld_->box()->setValue( newval );
	    else
	    {
		newval *= zfac;
		slcsel->z0fld_->box()->setValue( mNINT(newval) );
	    }
	}
    }

    slcsel->applyPush(0);
}


void timerTick( CallBacker* )
{
    if ( !inauto )
	return;
    if ( !paused )
	doAdvance( false );
    setTimer();
}


void setTimer()
{
    if ( !timer ) return;

    float val = dtfld_->getfValue();
    if ( mIsUdf(val) || val < 0.2 )
	val = 200;
    else
	val *= 1000;
    timer->start( mNINT(val), true );
}


bool rejectOK( CallBacker* )
{
    paused = true;
    inauto = false;
    return true;
}


const char* getTitle( uiSliceSel* ss )
{
    static BufferString title;
    title = "Control scrolling through ";
    if ( !ss->istsl_ )
	title += ss->isinl_ ? "Inlines" : "Crosslines";
    else
    {
	title += SI().zIsTime() ? "Time" : "Depth";
	title += " slices";
    }
    return title.buf();
}

    uiSliceSel*		slcsel;
    uiLabeledSpinBox*	stepfld_;
    uiLabeledComboBox*	typfld_;
    uiPushButton*	ctrlbut;
    uiPushButton*	backbut;
    uiGenInput*		dtfld_;
    const float		zfact;

    bool		paused;
    bool		inauto;
    Timer*		timer;

};


uiSliceSel::~uiSliceSel()
{
    delete applycb_;
    delete &updatemutex_;
    delete scrolldlg_;
}


void uiSliceSel::scrollPush( CallBacker* )
{
    if ( !scrolldlg_ )
	scrolldlg_ = new uiSliceScroll( this );
    scrolldlg_->show();
}


void uiSliceSel::applyPush( CallBacker* )
{
    if ( !updatemutex_.tryLock() )
	return;
    readInput();
    if ( applycb_ )
	applycb_->doCall(this);
    updatemutex_.unLock();
}


void uiSliceSel::readInput()
{
    const HorSampling& hs = maxcs_.hrg;
    Interval<int> inlrg, crlrg;
    hs.get( inlrg, crlrg );
    if ( inl0fld_ )
    {
	inlrg.start = inl0fld_->box()->getValue();
	inlrg.stop = isinl_ ? inlrg.start : inl1fld_->getValue();
	if ( !isinl_ && inlrg.start == inlrg.stop )
	    inlrg.stop += hs.step.inl;
    }

    crlrg.start = crl0fld_->box()->getValue();
    crlrg.stop = iscrl_ ? crlrg.start : crl1fld_->getValue();
    if ( !iscrl_ && crlrg.start == crlrg.stop )
	crlrg.stop += hs.step.crl;

    Interval<float> zrg;
    zrg.start = z0fld_->box()->getValue() / SI().zFactor();
    zrg.start = maxcs_.zrg.snap( zrg.start );
    if ( istsl_ )
	zrg.stop = zrg.start;
    else
    {
	zrg.stop = z1fld_->getValue() / SI().zFactor();
	zrg.sort();
	zrg.stop = maxcs_.zrg.snap( zrg.stop );
	if ( mIsEqual(zrg.start,zrg.stop,mDefEps) )
	    zrg.stop += maxcs_.zrg.step;
    }

    cs_.hrg.set( inlrg, crlrg );
    cs_.zrg.setFrom( zrg );

    SI().snap( cs_.hrg.start, BinID(0,0) );
    SI().snap( cs_.hrg.stop, BinID(0,0) );
}


void uiSliceSel::updateUI()
{
    if ( inl0fld_ )
    {
	Interval<int> inlrg( cs_.hrg.start.inl, cs_.hrg.stop.inl );
	StepInterval<int> maxinlrg( maxcs_.hrg.start.inl, maxcs_.hrg.stop.inl,
				    maxcs_.hrg.step.inl );
	setBoxValues( inl0fld_->box(), maxinlrg, inlrg.start );
	setBoxValues( inl1fld_, maxinlrg, inlrg.stop );
    }

    Interval<int> crlrg( cs_.hrg.start.crl, cs_.hrg.stop.crl );
    StepInterval<int> maxcrlrg( maxcs_.hrg.start.crl, maxcs_.hrg.stop.crl,
				maxcs_.hrg.step.crl );
    setBoxValues( crl0fld_->box(), maxcrlrg, crlrg.start );
    setBoxValues( crl1fld_, maxcrlrg, crlrg.stop );

    const float zfact( SI().zFactor() );
    Interval<int> zrg( mNINT(cs_.zrg.start*zfact), 
	    	       mNINT(cs_.zrg.stop*zfact) );
    StepInterval<int> maxzrg = 
		    StepInterval<int>( mNINT(maxcs_.zrg.start*zfact),
				       mNINT(maxcs_.zrg.stop*zfact),
				       mNINT(maxcs_.zrg.step*zfact) );
    setBoxValues( z0fld_->box(), maxzrg, zrg.start );
    setBoxValues( z1fld_, maxzrg, zrg.stop );
}


void uiSliceSel::setCubeSampling( const CubeSampling& cs )
{
    cs_ = cs;
    updateUI();
}


void uiSliceSel::setMaxCubeSampling( const CubeSampling& maxcs )
{
    maxcs_ = maxcs;
    updateUI();
}


bool uiSliceSel::acceptOK()
{
    readInput();
    return true;
}


void uiSliceSel::enableApplyButton( bool yn )
{
    if ( !applybut_ ) return;
    applybut_->display( yn );
}


void uiSliceSel::enableScrollButton( bool yn )
{
    if ( !scrollbut_ ) return;
    scrollbut_->display( yn );
}



uiSliceSelDlg::uiSliceSelDlg( uiParent* p, const CubeSampling& curcs,
			const CubeSampling& maxcs,
			const CallBack& acb, uiSliceSel::Type type )
    : uiDialog(p,uiDialog::Setup("Positioning",
				 "Specify the element's position","0.4.1")
	    	 .modal(type==uiSliceSel::Vol||type==uiSliceSel::TwoD))
{
    slicesel_ = new uiSliceSel( this, type );
    slicesel_->setMaxCubeSampling( maxcs );
    slicesel_->setCubeSampling( curcs );
    slicesel_->setApplyCB( acb );
    slicesel_->enableScrollButton( true );
}


bool uiSliceSelDlg::acceptOK( CallBacker* )
{
    return slicesel_->acceptOK();
}
