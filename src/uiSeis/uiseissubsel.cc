/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.cc,v 1.4 2004-07-29 21:41:26 bert Exp $
________________________________________________________________________

-*/

#include "uiseis2dsubsel.h"
#include "uigeninput.h"
#include "survinfo.h"
#include "iopar.h"
#include "separstr.h"
#include "cubesampling.h"
#include "uimsg.h"


uiSeis2DSubSel::uiSeis2DSubSel( uiParent* p, const BufferStringSet* lnms )
	: uiGroup( p, "2D seismics sub-selection" )
	, lnmsfld(0)
{
    selfld = new uiGenInput( this, "Select", BoolInpSpec("All","Part",true) );
    selfld->valuechanged.notify( mCB(this,uiSeis2DSubSel,selChg) );

    trcrgfld = new uiGenInput( this, "Trace range (start, stop, step)",
	    			IntInpIntervalSpec(true) );
    trcrgfld->setValue( 1, 0 ); trcrgfld->setValue( 1, 2 );
    trcrgfld->attach( alignedBelow, selfld );

    StepInterval<float> zrg = SI().zRange(true);
    BufferString fldtxt = "Z range "; fldtxt += SI().getZUnit();
    zfld = new uiGenInput( this, fldtxt, FloatInpIntervalSpec(true) );
    if ( !SI().zIsTime() )
	zfld->setValue( zrg );
    else
    {
	zfld->setValue( mNINT(zrg.start*1000), 0 );
	zfld->setValue( mNINT(zrg.stop*1000), 1 );
	zfld->setValue( mNINT(zrg.step*1000), 2 );
    }
    zfld->attach( alignedBelow, trcrgfld );

    static const BufferStringSet emptylnms;
    if ( !lnms ) lnms = &emptylnms;
    lnmsfld = new uiGenInput( this, "One line only",
			      StringListInpSpec(*lnms) );
    lnmsfld->setWithCheck( true );
    lnmsfld->attach( alignedBelow, zfld );

    setHAlignObj( selfld );
    setHCentreObj( selfld );

    mainObject()->finaliseStart.notify( mCB(this,uiSeis2DSubSel,doFinalise) );
}


void uiSeis2DSubSel::doFinalise( CallBacker* cb )
{
    selChg( cb );
}


bool uiSeis2DSubSel::isAll() const
{
    return selfld->getBoolValue();
}


void uiSeis2DSubSel::setInput( const StepInterval<int>& rg )
{
    trcrgfld->setValue( rg );
}


void uiSeis2DSubSel::setInput( const StepInterval<float>& rg )
{
    if ( SI().zIsTime() )
    {
	zfld->setValue( mNINT(rg.start*1000), 0 );
	zfld->setValue( mNINT(rg.stop*1000), 1 );
	zfld->setValue( mNINT(rg.step*1000), 2 );
    }
    else
	zfld->setValue( rg );
}


void uiSeis2DSubSel::setInput( const HorSampling& hs )
{
    StepInterval<int> trg( 1, hs.nrCrl(), 1 );
    trcrgfld->setValue( trg );
}


void uiSeis2DSubSel::selChg( CallBacker* )
{
    bool disp = !isAll();
    trcrgfld->display( disp );
    zfld->display( disp );
    lnmsfld->display( disp );
}


void uiSeis2DSubSel::usePar( const IOPar& iopar )
{
    //TODO other fields
    const char* res = iopar.find( SurveyInfo::sKeyZRange );
    if ( res  )
    {
	StepInterval<double> zrg = zfld->getDStepInterval();
	FileMultiString fms( res );
	const int sz = fms.size();
	if ( sz > 0 ) zrg.start = atof( fms[0] );
	if ( sz > 1 ) zrg.stop = atof( fms[1] );
	if ( sz > 2 ) zrg.step = atof( fms[2] );
	if ( SI().zIsTime() )
	{
	    zfld->setValue( mNINT(zrg.start*1000), 0 );
	    zfld->setValue( mNINT(zrg.stop*1000), 1 );
	    zfld->setValue( mNINT(zrg.step*1000), 2 );
	}
	else
	    zfld->setValue( zrg );
    }

    selChg( 0 );
}


bool uiSeis2DSubSel::fillPar( IOPar& iopar ) const
{
    //TODO other fields
    StepInterval<float> zrg;
    if ( !getZRange( zrg ) )
	return false;
    FileMultiString fms;
    fms += zrg.start; fms += zrg.stop; fms += zrg.step;
    iopar.set( SurveyInfo::sKeyZRange, (const char*)fms );

    return true;
}


bool uiSeis2DSubSel::getRange( StepInterval<int>& trg ) const
{
    const char* res = trcrgfld->text( 1 );
    if ( !res || !*res )
	{ trg.stop = mUndefIntVal; return false; }

    trg = trcrgfld->getIStepInterval();
    return true;
}


bool uiSeis2DSubSel::getZRange( StepInterval<float>& zrg ) const
{
    const StepInterval<float>& survzrg = SI().zRange(false);
    if ( isAll() )
	zrg = survzrg;
    else
    {
	assign( zrg, zfld->getFStepInterval() );
	if ( SI().zIsTime() )
	{
	    zrg.start *= 0.001;
	    zrg.stop *= 0.001;
	    if ( mIsUndefined(zrg.step) ) 
		zrg.step = survzrg.step;
	    else
		zrg.step *= 0.001;
	}
    }

    if ( zrg.start > zrg.stop || zrg.start > survzrg.stop ||
	 zrg.stop < survzrg.start )
    {
	uiMSG().error( "Z range not correct." );
	return false;
    }

    return true;
}


int uiSeis2DSubSel::expectedNrTraces() const
{
    StepInterval<int> trg; getRange( trg );
    return mIsUndefInt(trg.stop) ? -1 : trg.nrSteps() + 1;
}


int uiSeis2DSubSel::expectedNrSamples() const
{
    StepInterval<float> zrg; getZRange( zrg );
    return zrg.nrSteps() + 1;
}
