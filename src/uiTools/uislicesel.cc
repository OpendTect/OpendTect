/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.cc,v 1.11 2003-03-20 16:22:57 nanne Exp $
________________________________________________________________________

-*/

#include "uislicesel.h"
#include "cubesampling.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uibutton.h"
#include "survinfo.h"
#include "ranges.h"
#include "thread.h"


uiSliceSel::uiSliceSel( uiParent* p, const CubeSampling& cs_, 
			const CallBack& appcb, bool isvol )
        : uiDialog(p,uiDialog::Setup("Slice creation",
				     "Specify what you want to see",
				     0))
	, cs(*new CubeSampling)
	, inlrgfld(0)
	, inlfld(0)
	, crlrgfld(0)
	, crlfld(0)
	, zrgfld(0)
	, zfld(0)
	, doupdfld(0)
	, cschanged(this)
        , updatemutex( *new Threads::Mutex )
{
    int slctyp = cs_.hrg.start.inl == cs_.hrg.stop.inl ? 0 :
	         cs_.hrg.start.crl == cs_.hrg.stop.crl ? 1 : 
	         cs_.zrg.start == cs_.zrg.stop         ? 2 : 3 ;

    if ( isvol ) slctyp = 3;
    
    Interval<int> inlrg( cs_.hrg.start.inl, cs_.hrg.stop.inl );
    if ( !slctyp )
    {
	inlfld = new uiLabeledSpinBox( this, "Inline number" );
	inlfld->box()->setMinValue( SI().range().start.inl );
	inlfld->box()->setMaxValue( SI().range().stop.inl );
	inlfld->box()->setStep( SI().inlWorkStep() );
	inlfld->box()->setValue( inlrg.start );
	inlfld->box()->valueChanged.notify( mCB(this,uiSliceSel,csChanged) );
    }
    else
	inlrgfld = new uiGenInput( this, "Inline range",
		    IntInpIntervalSpec(inlrg) );

    Interval<int> crlrg( cs_.hrg.start.crl, cs_.hrg.stop.crl );
    if ( slctyp == 1 )
    {
	crlfld = new uiLabeledSpinBox( this, "Xline number" );
	crlfld->attach( alignedBelow, inlfld ? (uiGroup*) inlfld
					     : (uiGroup*) inlrgfld );
	crlfld->box()->setMinValue( SI().range().start.crl );
	crlfld->box()->setMaxValue( SI().range().stop.crl );
	crlfld->box()->setStep( SI().crlWorkStep() );
	crlfld->box()->setValue( crlrg.start );
	crlfld->box()->valueChanged.notify( mCB(this,uiSliceSel,csChanged) );
    }
    else
    {
	crlrgfld = new uiGenInput( this, "Xline range",
		    IntInpIntervalSpec(crlrg) );
	crlrgfld->attach( alignedBelow, inlfld ? (uiGroup*) inlfld 
					       : (uiGroup*) inlrgfld );
    }

    const float zfact( SI().zIsTime() ? 1000 : 1 );
    Interval<int> zrg( mNINT(cs_.zrg.start*zfact), mNINT(cs_.zrg.stop*zfact) );
    if ( slctyp == 2 )
    {
	BufferString zstr( "Slice " );
	zstr += SI().zIsTime() ? "time (" : "depth ("; 
	zstr += SI().getZUnit(); zstr += ")";
	zfld = new uiLabeledSpinBox( this, zstr );
	zfld->attach( alignedBelow, crlfld ? (uiGroup*) crlfld
					   : (uiGroup*) crlrgfld );
	zfld->box()->setMinValue( mNINT(SI().zRange().start*zfact) );
	zfld->box()->setMaxValue( mNINT(SI().zRange().stop*zfact) );
	zfld->box()->setStep( mNINT(SI().zRange().step*zfact) );
	zfld->box()->setValue( zrg.start );
	zfld->box()->valueChanged.notify( mCB(this,uiSliceSel,csChanged) );
    }
    else
    {
	zrgfld = new uiGenInput( this, "Time range (ms)", 
				 IntInpIntervalSpec(zrg) );
	zrgfld->attach( alignedBelow, crlfld ? (uiGroup*) crlfld 
					     : (uiGroup*) crlrgfld );
    }

    uiObj()->setTabOrder( inlfld ? (uiObject*)inlfld : (uiObject*)inlrgfld, 
	    		  crlfld ? (uiObject*)crlfld : (uiObject*)crlrgfld );
    uiObj()->setTabOrder( crlfld ? (uiObject*)crlfld : (uiObject*)crlrgfld, 
	    		  zfld ? (uiObject*)zfld : (uiObject*)zrgfld );

    if ( slctyp < 3 )
    {
	doupdfld = new uiCheckBox( this, "Immediate update" );
	doupdfld->setChecked( false );
	doupdfld->activated.notify( mCB(this,uiSliceSel,updateSel) );
	doupdfld->attach( alignedBelow, zfld ? (uiGroup*) zfld
					     : (uiGroup*) zrgfld );

	stepfld = new uiLabeledSpinBox( this, "Step" );
	int step = !slctyp ? SI().inlWorkStep() : 
		( slctyp == 1 ? SI().crlWorkStep()
			      : mNINT(SI().zRange().step*zfact) );
	stepfld->box()->setMinValue( step );
	stepfld->box()->setStep( step );
	int width = !slctyp ? inlfld->box()->maxValue() :
		( slctyp == 1 ? crlfld->box()->maxValue() 
			      :	zfld->box()->maxValue() );
	stepfld->box()->setMaxValue( width );
	stepfld->box()->valueChanged.notify( mCB(this,uiSliceSel,stepSel) );
	stepfld->attach( rightOf, doupdfld );
	uiObj()->setTabOrder( zfld ? (uiObject*)zfld : (uiObject*)zrgfld,
			      (uiObject*)doupdfld );
	uiObj()->setTabOrder( (uiObject*)doupdfld, (uiObject*)stepfld );
    }

    finaliseDone.notify( mCB(this,uiSliceSel,updateSel) );
    cschanged.notify( appcb );
}


uiSliceSel::~uiSliceSel()
{
    delete &cs;
    delete &updatemutex;
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
/*
    bool inlfldsens = inlfld ? inlfld->sensitive() : false;
    if ( inlfld ) inlfld->setSensitive(false);
    bool crlfldsens = crlfld ? crlfld->sensitive() : false;
    if ( crlfld ) crlfld->setSensitive(false);
    bool zfldsens = zfld ? zfld->sensitive() : false;
    if ( zfld ) zfld->setSensitive(false);
    bool inlrgfldsens = inlrgfld ? inlrgfld->sensitive() : false;
    if ( inlrgfld ) inlrgfld->setSensitive(false);
    bool crlrgfldsens = crlrgfld ? crlrgfld->sensitive() : false;
    if ( crlrgfld ) crlrgfld->setSensitive(false);
    bool zrgfldsens = zrgfld ? zrgfld->sensitive() : false;
    if ( zrgfld ) zrgfld->setSensitive(false);
    bool doupdfldsens = doupdfld ? doupdfld->sensitive() : false;
    if ( doupdfld ) doupdfld->setSensitive(false);
    bool stepfldsens = stepfld ? stepfld->sensitive() : false;
    if ( stepfld ) stepfld->setSensitive(false);
*/

    readInput();
    cschanged.trigger();
    updatemutex.unlock();

/*
    if ( inlfld ) inlfld->setSensitive(inlfldsens);
    if ( crlfld ) crlfld->setSensitive(crlfldsens);
    if ( zfld ) zfld->setSensitive(zfldsens);
    if ( inlrgfld ) inlrgfld->setSensitive(inlrgfldsens);
    if ( crlrgfld ) crlrgfld->setSensitive(crlrgfldsens);
    if ( zrgfld ) zrgfld->setSensitive(zrgfldsens);
    if ( doupdfld ) doupdfld->setSensitive(doupdfldsens);
    if ( stepfld ) stepfld->setSensitive(stepfldsens);
*/
}


void uiSliceSel::stepSel( CallBacker* )
{
    int newstep = stepfld->box()->getIntValue();
    if ( inlfld ) inlfld->box()->setStep( newstep );
    if ( crlfld ) crlfld->box()->setStep( newstep );
    if ( zfld ) zfld->box()->setStep( newstep );
}


void uiSliceSel::readInput()
{
    if ( inlfld )
        cs.hrg.start.inl = cs.hrg.stop.inl = inlfld->box()->getIntValue();
    else
    {
	Interval<int> intv = inlrgfld->getIInterval();
	SI().checkInlRange( intv );
	if ( intv.start > intv.stop )
	    Swap( intv.start, intv.stop );
	if ( intv.start == intv.stop )
	    intv.stop += SI().inlWorkStep();
        cs.hrg.start.inl = intv.start;
        cs.hrg.stop.inl = intv.stop;
    }

    if ( crlfld )
        cs.hrg.start.crl = cs.hrg.stop.crl = crlfld->box()->getIntValue();
    else
    {
	Interval<int> intv = crlrgfld->getIInterval();
	SI().checkCrlRange( intv );
	if ( intv.start > intv.stop )
	    Swap( intv.start, intv.stop );
	if ( intv.start == intv.stop )
	    intv.stop += SI().crlWorkStep();
        cs.hrg.start.crl = intv.start;
        cs.hrg.stop.crl = intv.stop;
    }

    if ( zfld )
        cs.zrg.start = cs.zrg.stop = zfld->box()->getIntValue() * .001;
    else
    {
	Interval<double> intv( zrgfld->getIntValue(0) * .001, 
			       zrgfld->getIntValue(1) * .001 );
	SI().checkZRange( intv );
	if ( intv.start > intv.stop )
            Swap( intv.start, intv.stop );
	if ( intv.start == intv.stop )
	    intv.stop += SI().zRange().step;
        cs.zrg.start = intv.start;
        cs.zrg.stop = intv.stop;
    }

    SI().snap( cs.hrg.start, BinID(0,0) );
    SI().snap( cs.hrg.stop, BinID(0,0) );
}


bool uiSliceSel::acceptOK( CallBacker* )
{
    readInput();
    return true;
}
