/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.cc,v 1.27 2004-11-18 16:15:23 bert Exp $
________________________________________________________________________

-*/

#include "uiseissubsel.h"
#include "uibinidsubsel.h"
#include "uiseisioobjinfo.h"
#include "uigeninput.h"
#include "seistrcsel.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "iopar.h"
#include "ioobj.h"
#include "separstr.h"
#include "cubesampling.h"
#include "keystrs.h"
#include "uimsg.h"


uiSeisSubSel::uiSeisSubSel( uiParent* p, bool for_new_entry, bool wstep,
			    bool m2dln )
    	: uiGroup(p,"Gen seis subsel")
	, is2d_(false)
{
    sel2d = new uiSeis2DSubSel( this, for_new_entry, m2dln );
    sel3d = new uiBinIDSubSel( this, uiBinIDSubSel::Setup()
				     .withtable(false).withz(true)
				     .withstep(wstep) );
    sel3d->attach( alignedWith, sel2d );

    setHAlignObj( sel2d );
    mainObject()->finaliseDone.notify( mCB(this,uiSeisSubSel,typChg) );
}


void uiSeisSubSel::typChg( CallBacker* )
{
    sel2d->display( is2d_ );
    sel3d->display( !is2d_ );
}


void uiSeisSubSel::clear()
{
    if ( is2d_ )
	sel2d->clear();
    else
	sel3d->clear();
}


void uiSeisSubSel::setInput( const HorSampling& hs )
{
    if ( is2d_ )
	sel2d->setInput( hs );
    else
	sel3d->setInput( hs );
}


void uiSeisSubSel::setInput( const StepInterval<float>& zrg )
{
    sel2d->setInput( zrg );
    sel3d->setInput( zrg );
}


void uiSeisSubSel::setInput( const CubeSampling& cs )
{
    setInput( cs.hrg );
    setInput( cs.zrg );
}


void uiSeisSubSel::setInput( const IOObj& ioobj )
{
    set2D( SeisTrcTranslator::is2D(ioobj) );
    if ( is2d_ )
	sel2d->setInput( ioobj );
    else
    {
	uiSeisIOObjInfo oinf(ioobj,false); CubeSampling cs;
	if ( !oinf.getRanges(cs) )
	    clear();
	else
	    setInput( cs );
    }
}


bool uiSeisSubSel::isAll() const
{
    return is2d_ ? sel2d->isAll() : sel3d->isAll();
}


bool uiSeisSubSel::getSampling( HorSampling& hs ) const
{
    if ( is2d_ )
    {
	StepInterval<int> trcrg;
	if ( !sel2d->getRange( trcrg ) )
	    return false;
	hs.start.crl = trcrg.start;
	hs.stop.crl = trcrg.stop;
	hs.step.crl = trcrg.step;
    }
    else
    {
	CubeSampling cs;
	if ( !sel3d->getSampling(cs) )
	    return false;
	hs = cs.hrg;
    }
    return true;
}


bool uiSeisSubSel::getZRange( Interval<float>& zrg ) const
{
    return is2d_ ? sel2d->getZRange(zrg) : sel3d->getZRange(zrg);
}


bool uiSeisSubSel::fillPar( IOPar& iop ) const
{
    return is2d_ ? sel2d->fillPar(iop) : sel3d->fillPar(iop);
}


void uiSeisSubSel::usePar( const IOPar& iop )
{
    if ( is2d_ )
	sel2d->usePar( iop );
    else
	sel3d->usePar( iop );
}


int uiSeisSubSel::expectedNrSamples() const
{
    return is2d_ ? sel2d->expectedNrSamples() : sel3d->expectedNrSamples();
}


int uiSeisSubSel::expectedNrTraces() const
{
    return is2d_ ? sel2d->expectedNrTraces() : sel3d->expectedNrTraces();
}


void uiSeisSubSel::notifySing2DLineSel( const CallBack& cb )
{
    sel2d->singLineSel.notify( cb );
}

bool uiSeisSubSel::isSing2DLine() const
{
    return is2d_ ? sel2d->isSingLine() : false;
}


const char* uiSeisSubSel::selectedLine() const
{
    return is2d_ ? sel2d->selectedLine() : "";
}


void uiSeisSubSel::setSelectedLine( const char* nm )
{
    if ( is2d_ )
	sel2d->setSelectedLine( nm );
}


static const BufferStringSet emptylnms;


uiSeis2DSubSel::uiSeis2DSubSel( uiParent* p, bool for_new_entry, bool mln )
	: uiGroup( p, "2D seismics sub-selection" )
	, lnmsfld(0)
	, lnmfld(0)
    	, multiln(mln)
	, lineSel(this)
	, singLineSel(this)
{
    if ( for_new_entry )
	lnmfld = new uiGenInput( this, "Store in Set as" );
    else
    {
	lnmsfld = new uiGenInput( this, multiln ? "One line only" : "Line name",
				  StringListInpSpec(emptylnms) );
	if ( multiln )
	{
	    lnmsfld->setWithCheck( true );
	    lnmsfld->checked.notify( mCB(this,uiSeis2DSubSel,singLineChg) );
	}
	lnmsfld->valuechanged.notify( mCB(this,uiSeis2DSubSel,lineChg) );
    }

    selfld = new uiGenInput( this, "Select", BoolInpSpec("All","Part",true) );
    selfld->valuechanged.notify( mCB(this,uiSeis2DSubSel,selChg) );
    selfld->attach( alignedBelow, lnmfld ? lnmfld : lnmsfld );

    trcrgfld = new uiGenInput( this, "Trace number range (start, stop, step)",
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


bool uiSeis2DSubSel::isSingLine() const
{
    return lnmfld || !multiln || lnmsfld->isChecked();
}


const char* uiSeis2DSubSel::selectedLine() const
{
    return isSingLine() ? (lnmfld ? lnmfld : lnmsfld)->text() : "";
}


void uiSeis2DSubSel::setSelectedLine( const char* nm )
{
    if ( lnmfld )
	lnmfld->setText( nm );
    else
	lnmsfld->setText( nm );
}


void uiSeis2DSubSel::clear()
{
    trcrgfld->setValue(1,0); trcrgfld->setText("",1); trcrgfld->setValue(1,2);
    setInput( SI().zRange() );
    selfld->setValue( true );
    if ( lnmfld )
	lnmfld->setText( "" );
    else
    {
	if ( multiln )
	    lnmsfld->setChecked( false );
	lnmsfld->newSpec( StringListInpSpec(emptylnms), 0 );
    }
    selChg( 0 );
}


void uiSeis2DSubSel::setInput( const StepInterval<int>& rg )
{
    trcrgfld->setValue( rg );
}


void uiSeis2DSubSel::setInput( const Interval<float>& zrg )
{
    if ( !SI().zIsTime() )
	zfld->setValue( zrg );
    else
    {
	zfld->setValue( mNINT(zrg.start*1000), 0 );
	zfld->setValue( mNINT(zrg.stop*1000), 1 );
	mDynamicCastGet(const StepInterval<float>*,szrg,&zrg);
	if ( szrg )
	    zfld->setValue( mNINT(szrg->step*1000), 2 );
    }
}


void uiSeis2DSubSel::setInput( const HorSampling& hs )
{
    StepInterval<int> trg( 1, hs.nrCrl(), 1 );
    if ( mIsUndefInt(hs.stop.crl) )
	trg.stop = mUndefIntVal;
    trcrgfld->setValue( trg );
}


void uiSeis2DSubSel::setInput( const IOObj& ioobj )
{
    uiSeisIOObjInfo oinf(ioobj,false); CubeSampling cs;
    if ( !oinf.getRanges(cs) )
	{ clear(); return; }

    setInput( cs.hrg );
    setInput( cs.zrg );

    if ( !lnmsfld ) return;

    const BufferString prevlnm( selectedLine() );

    BufferStringSet lnms; oinf.getLineNames( lnms );
    lnmsfld->newSpec( StringListInpSpec(lnms), 0 );
    const bool prevok = prevlnm != "" && lnms.indexOf(prevlnm) >= 0;
    if ( multiln )
	lnmsfld->setChecked( prevok );

    if ( prevok )
	lnmsfld->setText( prevlnm );
}


void uiSeis2DSubSel::lineChg( CallBacker* )
{
    lineSel.trigger();
}


void uiSeis2DSubSel::singLineChg( CallBacker* )
{
    singLineSel.trigger();
}


void uiSeis2DSubSel::selChg( CallBacker* )
{
    bool disp = !isAll();
    trcrgfld->display( disp );
    zfld->display( disp );
}


void uiSeis2DSubSel::usePar( const IOPar& iopar )
{
    LineKey lk; lk.usePar( iopar, false );
    BufferString lnm( lk.lineName() );
    if ( lnmfld )
	lnmfld->setText( lnm );
    else
    {
	lnmsfld->setText( lnm );
	if ( multiln ) lnmsfld->setChecked( lnm != "" );
    }

    StepInterval<int> trcrg = trcrgfld->getIStepInterval();
    iopar.get( sKey::FirstCrl, trcrg.start );
    iopar.get( sKey::LastCrl, trcrg.stop );
    iopar.get( sKey::StepCrl, trcrg.step );
    trcrgfld->setValue(trcrg);

    const char* res = iopar.find( sKey::ZRange );
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

    res = iopar.find( sKey::BinIDSel );
    selfld->setValue( !res || *res != *sKey::Range );

    selChg( 0 );
}


bool uiSeis2DSubSel::fillPar( IOPar& iopar ) const
{
    const bool isall = isAll();
    iopar.set( sKey::BinIDSel, isall ? sKey::No : sKey::Range );

    BufferString lnm( selectedLine() );
    if ( lnm == "" )
	iopar.removeWithKey( sKey::LineKey );
    else
	iopar.set( sKey::LineKey, lnm );

    iopar.set( sKey::FirstInl, 0 );
    iopar.set( sKey::LastInl, mUndefIntVal );
    iopar.set( sKey::StepCrl, 1 );

    if ( !isall )
    {
	StepInterval<int> trcrg = trcrgfld->getIStepInterval();
	iopar.set( sKey::FirstCrl, trcrg.start );
	iopar.set( sKey::LastCrl, trcrg.stop );
	iopar.set( sKey::StepCrl, trcrg.step );

	StepInterval<float> zrg;
	if ( !getZRange( zrg ) )
	    return false;
	FileMultiString fms;
	fms += zrg.start; fms += zrg.stop; fms += zrg.step;
	iopar.set( sKey::ZRange, (const char*)fms );
    }

    return true;
}


bool uiSeis2DSubSel::getRange( StepInterval<int>& trg ) const
{
    trg.start = 0;
    const char* res = trcrgfld->text( 1 );
    if ( isAll() || !res || !*res )
    {
	trg.stop = mUndefIntVal;
	return true;
    }

    trg = trcrgfld->getIStepInterval();
    return true;
}


bool uiSeis2DSubSel::getZRange( Interval<float>& zrg ) const
{
    mDynamicCastGet(StepInterval<float>*,szrg,&zrg);
    const StepInterval<float>& survzrg = SI().zRange(false);
    if ( isAll() )
    {
	assign( zrg, survzrg );
	if ( szrg ) szrg->step = survzrg.step;
    }
    else
    {
	StepInterval<float> fldzrg = zfld->getFStepInterval();
	if ( SI().zIsTime() )
	{
	    fldzrg.start *= 0.001;
	    fldzrg.stop *= 0.001;
	    if ( mIsUndefined(fldzrg.step) ) 
		fldzrg.step = survzrg.step;
	    else
		fldzrg.step *= 0.001;
	}
	assign( zrg, fldzrg );
	if ( szrg ) szrg->step = fldzrg.step;
    }

    if ( zrg.start > zrg.stop || zrg.start > survzrg.stop ||
	 zrg.stop < survzrg.start )
    {
	assign( zrg, survzrg );
	uiMSG().error( "Z range not correct." );
	return false;
    }

    return true;
}


int uiSeis2DSubSel::expectedNrTraces() const
{
    StepInterval<int> trg;
    if ( getRange(trg) )
	return mIsUndefInt(trg.stop) ? -1 : trg.nrSteps() + 1;
    return -1;
}


int uiSeis2DSubSel::expectedNrSamples() const
{
    StepInterval<float> zrg; getZRange( zrg );
    return zrg.nrSteps() + 1;
}
