/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.cc,v 1.3 2002-05-09 05:27:27 kristofer Exp $
________________________________________________________________________

-*/

#include "uislicesel.h"
#include "cubesampling.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "survinfo.h"
#include "ranges.h"


uiSliceSel::uiSliceSel( uiParent* p, CubeSampling* cs )
        : uiDialog(p,
            uiDialog::Setup("Slice creation","Specify what you want to see",0))
	, cs(cs)
{
    slctyp = cs->hrg.start.inl == cs->hrg.stop.inl ? 0 :
	     cs->hrg.start.crl == cs->hrg.stop.crl ? 1 : 
	     cs->zrg.start == cs->zrg.stop         ? 2 : 3 ;
    
    Interval<int> inlintv( cs->hrg.start.inl, cs->hrg.stop.inl );
    inlrgfld = new uiGenInput( this, "Inline range",
                IntInpIntervalSpec(inlintv) );
    inlfld = new uiLabeledSpinBox( this, "Inline number" );

    Interval<int> crlintv( cs->hrg.start.crl, cs->hrg.stop.crl );
    crlrgfld = new uiGenInput( this, "Xline range",
                IntInpIntervalSpec(crlintv) );
    crlrgfld->attach( alignedBelow, slctyp
	    			? (uiGroup*) inlrgfld : (uiGroup*) inlfld );
    crlfld = new uiLabeledSpinBox( this, "Xline number" );
    crlfld->attach( alignedBelow, inlrgfld );

    Interval<int> zrg((int)(cs->zrg.start*1000+.5),(int)(cs->zrg.stop*1000+.5));
    zrgfld = new uiGenInput( this, "Time range (ms)", IntInpIntervalSpec(zrg) );
    zrgfld->attach( alignedBelow, slctyp == 1
	    			? (uiGroup*) crlfld : (uiGroup*) crlrgfld );
    zfld = new uiLabeledSpinBox( this, "Slice time (ms)" );
    zfld->attach( alignedBelow, crlrgfld );

    inlfld->box()->setMinValue( SI().range().start.inl );
    inlfld->box()->setMaxValue( SI().range().stop.inl );
    inlfld->box()->setStep( SI().step().inl );
    inlfld->box()->setValue( cs->hrg.start.inl );
    crlfld->box()->setMinValue( SI().range().start.crl );
    crlfld->box()->setMaxValue( SI().range().stop.crl );
    crlfld->box()->setStep( SI().step().crl );
    crlfld->box()->setValue( cs->hrg.start.crl );
    zfld->box()->setMinValue( (int)(SI().zRange().start*1000+.5) );
    zfld->box()->setMaxValue( (int)(SI().zRange().stop*1000+.5) );
    zfld->box()->setStep( (int)(SI().zRange().step*1000+.5) );
    zfld->box()->setValue( (int)(cs->zrg.start*1000+.5) );

    finaliseDone.notify( mCB(this,uiSliceSel,selChg) );
}


void uiSliceSel::selChg( CallBacker* )
{
    inlrgfld->display( slctyp != 0 );
    inlfld->display( slctyp == 0 );
    crlrgfld->display( slctyp != 1 );
    crlfld->display( slctyp == 1 );
    zrgfld->display( slctyp != 2 );
    zfld->display( slctyp == 2 );
}


bool uiSliceSel::acceptOK( CallBacker* )
{
    if ( slctyp == 0 )
        cs->hrg.start.inl = cs->hrg.stop.inl = inlfld->box()->getIntValue();
    else
    {
	Interval<int> intv = inlrgfld->getIInterval();
	SI().checkInlRange( intv );
	if ( intv.start > intv.stop )
	    Swap( intv.start, intv.stop );
        cs->hrg.start.inl = intv.start;
        cs->hrg.stop.inl = intv.stop;
    }

    if ( slctyp == 1 )
        cs->hrg.start.crl = cs->hrg.stop.crl = crlfld->box()->getIntValue();
    else
    {
	Interval<int> intv = crlrgfld->getIInterval();
	SI().checkCrlRange( intv );
	if ( intv.start > intv.stop )
	    Swap( intv.start, intv.stop );
        cs->hrg.start.crl = intv.start;
        cs->hrg.stop.crl = intv.stop;
    }

    if ( slctyp == 2 )
        cs->zrg.start = cs->zrg.stop = zfld->box()->getIntValue() * .001;
    else
    {
	Interval<double> intv( zrgfld->getIntValue(0) * .001, 
			       zrgfld->getIntValue(1) * .001 );
	SI().checkZRange( intv );
	if ( intv.start > intv.stop )
            Swap( intv.start, intv.stop );
        cs->zrg.start = intv.start;
        cs->zrg.stop = intv.stop;
    }

    SI().snap( cs->hrg.start, BinID(0,0) );
    SI().snap( cs->hrg.stop, BinID(0,0) );
    return true;
}

