/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.cc,v 1.25 2005-04-15 12:32:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "uislicesel.h"
#include "cubesampling.h"
#include "uispinbox.h"
#include "uibutton.h"
#include "survinfo.h"
#include "thread.h"


uiSliceSel::uiSliceSel( uiParent* p, const CubeSampling& curcs_,
			const CubeSampling& maxcs_,
			const CallBack& acb, Type type )
    : uiDialog(p,uiDialog::Setup("Positioning",
				 "Specify the element's position",
				 "").modal(type==Vol||type==TwoD))
    , cs(*new CubeSampling(curcs_))
    , curcs(curcs_)
    , maxcs(maxcs_)
    , inl0fld(0)
    , updatemutex(*new Threads::Mutex)
    , appcb(*new CallBack(acb))
{
    isinl = type == Inl;
    iscrl = type == Crl;
    istsl = type == Tsl;
    isvol = type == Vol;
    is2d = type == TwoD;

    if ( !is2d )
	createInlFld();

    createCrlFld();
    createZFld();

    if ( inl0fld )
	mainObject()->setTabOrder( (uiObject*)inl0fld, (uiObject*)crl0fld );
    mainObject()->setTabOrder( (uiObject*)crl0fld, (uiObject*)z0fld );

    uiButton* applybut = new uiPushButton( this, "Apply" );
    applybut->activated.notify( mCB(this,uiSliceSel,applyPush) );
    mainObject()->setTabOrder( (uiObject*)z0fld, (uiObject*)applybut );
    applybut->attach( alignedBelow, z0fld );
    if ( !isvol && !is2d )
    {
	uiButton* scrollbut = new uiPushButton( this, "Scroll ..." );
	scrollbut->activated.notify( mCB(this,uiSliceSel,scrollPush) );
	scrollbut->attach( rightOf, isinl ? inl0fld : (iscrl?crl0fld:z0fld) );
    }
}


void uiSliceSel::createInlFld()
{
    Interval<int> inlrg( curcs.hrg.start.inl, curcs.hrg.stop.inl );
    StepInterval<int> maxinlrg( maxcs.hrg.start.inl, maxcs.hrg.stop.inl,
				maxcs.hrg.step.inl );
    BufferString label( isinl ? "Inline nr" : "Inline range" );
    inl0fld = new uiLabeledSpinBox( this, label );
    setBoxValues( inl0fld->box(), maxinlrg, inlrg.start );
    inl1fld = new uiSpinBox( this );
    setBoxValues( inl1fld, maxinlrg, inlrg.stop );
    inl1fld->attach( rightTo, inl0fld );
    inl1fld->display( !isinl );
}


void uiSliceSel::createCrlFld()
{
    Interval<int> crlrg( curcs.hrg.start.crl, curcs.hrg.stop.crl );
    StepInterval<int> maxcrlrg( maxcs.hrg.start.crl, maxcs.hrg.stop.crl,
				maxcs.hrg.step.crl );
    BufferString label = is2d ? "Trace range" 
			      : ( iscrl ? "Xline nr" : "Xline range" );
    crl0fld = new uiLabeledSpinBox( this, label );
    setBoxValues( crl0fld->box(), maxcrlrg, crlrg.start );
    crl1fld = new uiSpinBox( this );
    setBoxValues( crl1fld, maxcrlrg, crlrg.stop );
    crl1fld->attach( rightTo, crl0fld );
    crl1fld->display( !iscrl );
    if ( inl0fld ) crl0fld->attach( alignedBelow, inl0fld );
}


void uiSliceSel::createZFld()
{
    const float zfact( SI().zFactor() );
    Interval<int> zrg( mNINT(curcs.zrg.start*zfact), 
	    	       mNINT(curcs.zrg.stop*zfact) );
    BufferString label = SI().zIsTime() ? "Time " : "Depth ";
    if ( !istsl ) label += "range "; label += SI().getZUnit();
    StepInterval<int> maxzrg = 
		    StepInterval<int>( mNINT(maxcs.zrg.start*zfact),
				       mNINT(maxcs.zrg.stop*zfact),
				       mNINT(maxcs.zrg.step*zfact) );

    z0fld = new uiLabeledSpinBox( this, label );
    setBoxValues( z0fld->box(), maxzrg, zrg.start );
    z1fld = new uiSpinBox( this );
    setBoxValues( z1fld, maxzrg, zrg.stop );
    z1fld->attach( rightTo, z0fld );
    z1fld->display( !istsl );
    z0fld->attach( alignedBelow, crl0fld );
}


uiSliceSel::~uiSliceSel()
{
    delete &cs;
    delete &appcb;
    delete &updatemutex;
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

uiSliceScroll( uiSliceSel* p, const CallBack& c )
	: uiDialog(p,uiDialog::Setup("Scrolling",
				 "Control scrolling through the data","0.4.2")
		.oktext("").canceltext("Dismiss"))
	, appcb(c)
{
}

    CallBack	appcb;

};


void uiSliceSel::scrollPush( CallBacker* )
{
    uiSliceScroll dlg( this, appcb );
    dlg.go();
}


void uiSliceSel::applyPush( CallBacker* )
{
    if ( !updatemutex.tryLock() )
	return;
    readInput();
    appcb.doCall(this);
    updatemutex.unlock();
}


void uiSliceSel::readInput()
{
    const HorSampling& hs = SI().sampling(true).hrg;
    Interval<int> inlrg, crlrg;
    hs.get( inlrg, crlrg );
    if ( inl0fld )
    {
	inlrg.start = inl0fld->box()->getValue();
	inlrg.stop = isinl ? inlrg.start : inl1fld->getValue();
	if ( !isinl && inlrg.start == inlrg.stop )
	    inlrg.stop += hs.step.inl;
    }

    crlrg.start = crl0fld->box()->getValue();
    crlrg.stop = iscrl ? crlrg.start : crl1fld->getValue();
    if ( !iscrl && crlrg.start == crlrg.stop )
	crlrg.stop += hs.step.crl;

    Interval<float> zrg;
    zrg.start = z0fld->box()->getValue() / SI().zFactor();
    SI().snapZ( zrg.start, 0 );
    if ( istsl )
	zrg.stop = zrg.start;
    else
    {
	zrg.stop = z1fld->getValue() / SI().zFactor();
	zrg.sort();
	SI().snapZ( zrg.stop, 1 );
	if ( mIsEqual(zrg.start,zrg.stop,mDefEps) )
	    zrg.stop += SI().zRange().step;
    }

    cs.hrg.set( inlrg, crlrg );
    assign(cs.zrg,zrg);

    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
}


bool uiSliceSel::acceptOK( CallBacker* )
{
    readInput();
    return true;
}
