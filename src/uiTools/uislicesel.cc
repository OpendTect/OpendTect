/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.cc,v 1.22 2005-03-03 16:43:38 cvsnanne Exp $
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
			const CallBack& appcb, Type type )
    : uiDialog(p,uiDialog::Setup("Positioning",
				 "Specify what you want to see",
				 0).modal(type==Vol||type==TwoD))
    , cs(*new CubeSampling(curcs_))
    , curcs(curcs_)
    , maxcs(maxcs_)
    , doupdfld(0)
    , inl0fld(0)
    , cschanged(this)
    , updatemutex(*new Threads::Mutex)
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

    if ( !isvol && !is2d )
	createUpdateFld();

    finaliseDone.notify( mCB(this,uiSliceSel,updateSel) );
    cschanged.notify( appcb );
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


void uiSliceSel::createUpdateFld()
{
    doupdfld = new uiCheckBox( this, "Immediate update" );
    doupdfld->setChecked( false );
    doupdfld->activated.notify( mCB(this,uiSliceSel,updateSel) );
    doupdfld->attach( alignedBelow, z0fld );

    stepfld = new uiLabeledSpinBox( this, "Step" );
    const int zstep = mNINT( SI().zRange().step*SI().zFactor() );
    const int step = isinl ? SI().inlStep()
			   : (iscrl ? SI().crlStep() : zstep );
    stepfld->box()->setMinValue( step );
    stepfld->box()->setStep( step, true );
    const int width = isinl ? inl0fld->box()->maxValue() 
			    : ( iscrl ? crl0fld->box()->maxValue() 
				      : z0fld->box()->maxValue() );
    stepfld->box()->setMaxValue( width );
    stepfld->box()->valueChanged.notify( mCB(this,uiSliceSel,stepSel) );
    stepfld->attach( rightOf, doupdfld );
    mainObject()->setTabOrder( (uiObject*)z0fld, (uiObject*)doupdfld );
    mainObject()->setTabOrder( (uiObject*)doupdfld, (uiObject*)stepfld );
}


uiSliceSel::~uiSliceSel()
{
    delete &cs;
    delete &updatemutex;
}


void uiSliceSel::setBoxValues( uiSpinBox* box, const StepInterval<int>& intv, 
			       int curval )
{
    box->setMinValue( intv.start );
    box->setMaxValue( intv.stop );
    box->setStep( intv.step, true );
    box->setValue( curval );
    box->valueChanged.notify( mCB(this,uiSliceSel,csChanged) );
}


void uiSliceSel::updateSel( CallBacker* )
{
    if ( !doupdfld ) return;
    bool doupdate = doupdfld->isChecked();
    stepfld->setSensitive( doupdate );
}


void uiSliceSel::csChanged( CallBacker* )
{
    if ( !doupdfld || !doupdfld->isChecked() ) return;

    if ( !updatemutex.tryLock() )
	return;
    readInput();
    cschanged.trigger();
    updatemutex.unlock();
}


void uiSliceSel::stepSel( CallBacker* )
{
    int newstep = stepfld->box()->getValue();
    if ( isinl )
	inl0fld->box()->setStep( newstep, true );
    else if ( iscrl )
	crl0fld->box()->setStep( newstep, true );
    else if ( istsl )
	z0fld->box()->setStep( newstep, true );
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
