/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.cc,v 1.13 2003-08-12 14:37:54 nanne Exp $
________________________________________________________________________

-*/

#include "uislicesel.h"
#include "cubesampling.h"
#include "uispinbox.h"
#include "uibutton.h"
#include "survinfo.h"
#include "thread.h"


uiSliceSel::uiSliceSel( uiParent* p, const CubeSampling& cs_, 
			const CallBack& appcb, bool isvol_ )
        : uiDialog(p,uiDialog::Setup("Slice creation",
				     "Specify what you want to see",
				     0))
	, cs(*new CubeSampling)
	, doupdfld(0)
	, cschanged(this)
        , updatemutex( *new Threads::Mutex )
{
    isinl = cs_.hrg.start.inl == cs_.hrg.stop.inl;
    iscrl = cs_.hrg.start.crl == cs_.hrg.stop.crl;
    istsl = cs_.zrg.start == cs_.zrg.stop;
    isvol = isvol_;
    
    Interval<int> inlrg( cs_.hrg.start.inl, cs_.hrg.stop.inl );
    inl0fld = new uiLabeledSpinBox( this, "Inline range" );
    setBoxValues( inl0fld->box(), SI().inlRange(), inlrg.start );
    inl1fld = new uiSpinBox( this );
    setBoxValues( inl1fld, SI().inlRange(), inlrg.stop );
    inl1fld->attach( rightTo, inl0fld );
    inl1fld->display( !isinl );

    Interval<int> crlrg( cs_.hrg.start.crl, cs_.hrg.stop.crl );
    crl0fld = new uiLabeledSpinBox( this, "Xline range" );
    setBoxValues( crl0fld->box(), SI().crlRange(), crlrg.start );
    crl0fld->attach( alignedBelow, inl0fld );
    crl1fld = new uiSpinBox( this );
    crl1fld->attach( rightTo, crl0fld );
    setBoxValues( crl1fld, SI().crlRange(), crlrg.stop );
    crl1fld->display( !iscrl );

    const float zfact( SI().zIsTime() ? 1000 : 1 );
    Interval<int> zrg( mNINT(cs_.zrg.start*zfact), mNINT(cs_.zrg.stop*zfact) );
    BufferString zstr; zstr = SI().zIsTime() ? "Time range (" : "Depth range (";
    zstr += SI().getZUnit(); zstr += ")";
    z0fld = new uiLabeledSpinBox( this, zstr );
    StepInterval<int> totzrg = 
	    StepInterval<int>( mNINT(SI().zRange().start*zfact),
			       mNINT(SI().zRange().stop*zfact),
			       mNINT(SI().zRange().step*zfact) );
    setBoxValues( z0fld->box(), totzrg, zrg.start );
    z0fld->attach( alignedBelow, crl0fld );
    z1fld = new uiSpinBox( this, zstr );
    z1fld->attach( rightTo, z0fld );
    setBoxValues( z1fld, totzrg, zrg.stop );
    z1fld->display( !istsl );

    mainObject()->setTabOrder( (uiObject*)inl0fld, (uiObject*)crl0fld );
    mainObject()->setTabOrder( (uiObject*)crl0fld, (uiObject*)z0fld );

    if ( !isvol )
    {
	doupdfld = new uiCheckBox( this, "Immediate update" );
	doupdfld->setChecked( false );
	doupdfld->activated.notify( mCB(this,uiSliceSel,updateSel) );
	doupdfld->attach( alignedBelow, z0fld );

	stepfld = new uiLabeledSpinBox( this, "Step" );
	int step = isinl || iscrl ? SI().getStep(isinl,true) : totzrg.step;
	stepfld->box()->setMinValue( step );
	stepfld->box()->setStep( step );
	int width = isinl ? inl0fld->box()->maxValue() 
	    		  : ( iscrl ? crl0fld->box()->maxValue() 
			            : z0fld->box()->maxValue() );
	stepfld->box()->setMaxValue( width );
	stepfld->box()->valueChanged.notify( mCB(this,uiSliceSel,stepSel) );
	stepfld->attach( rightOf, doupdfld );
	mainObject()->setTabOrder( (uiObject*)z0fld, (uiObject*)doupdfld );
	mainObject()->setTabOrder( (uiObject*)doupdfld, (uiObject*)stepfld );
    }

    finaliseDone.notify( mCB(this,uiSliceSel,updateSel) );
    cschanged.notify( appcb );
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
    box->setStep( intv.step );
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
    int newstep = stepfld->box()->getIntValue();
    if ( isinl )
	inl0fld->box()->setStep( newstep );
    else if ( iscrl )
	crl0fld->box()->setStep( newstep );
    else if ( istsl )
	z0fld->box()->setStep( newstep );
}


void uiSliceSel::readInput()
{
    StepInterval<int> intv;
    intv.start = inl0fld->box()->getIntValue();
    intv.stop = isinl ? intv.start : inl1fld->getIntValue();
    SI().checkInlRange( intv );
    if ( intv.start > intv.stop )
	Swap( intv.start, intv.stop );
    if ( !isinl && intv.start == intv.stop )
	intv.stop += SI().inlWorkStep();
    cs.hrg.start.inl = intv.start;
    cs.hrg.stop.inl = intv.stop;

    
    intv.start = crl0fld->box()->getIntValue();
    intv.stop = iscrl ? intv.start : crl1fld->getIntValue();
    SI().checkCrlRange( intv );
    if ( intv.start > intv.stop )
	Swap( intv.start, intv.stop );
    if ( !iscrl && intv.start == intv.stop )
	intv.stop += SI().crlWorkStep();
    cs.hrg.start.crl = intv.start;
    cs.hrg.stop.crl = intv.stop;


    Interval<double> zintv;
    zintv.start = z0fld->box()->getIntValue() * .001;
    zintv.stop = istsl ? zintv.start : z1fld->getIntValue() * .001;
    SI().checkZRange( zintv );
    if ( zintv.start > zintv.stop )
	Swap( zintv.start, zintv.stop );
    if ( !istsl && zintv.start == zintv.stop )
	zintv.stop += SI().zRange().step;
    cs.zrg.start = zintv.start;
    cs.zrg.stop = zintv.stop;

    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
}


bool uiSliceSel::acceptOK( CallBacker* )
{
    readInput();
    return true;
}
