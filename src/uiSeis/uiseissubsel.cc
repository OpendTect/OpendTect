/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.cc,v 1.39 2006-09-21 17:47:52 cvsnanne Exp $
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
    {
	uiSeis2DSubSel::PosData data = sel2d->getInput();
	data.trcrg_ = hs.crlRange();
	sel2d->setInput( data );
    }
    else
    {
	uiBinIDSubSel::Data data = sel3d->getInput();
	data.cs_.hrg = hs;
	sel3d->setInput( data );
    }
}


void uiSeisSubSel::setInput( const StepInterval<float>& zrg )
{
    if ( is2d_ )
    {
	uiSeis2DSubSel::PosData data = sel2d->getInput();
	data.zrg_ = zrg;
	sel2d->setInput( data );
    }
    else
    {
	uiBinIDSubSel::Data data = sel3d->getInput();
	data.cs_.zrg = zrg;
	sel3d->setInput( data );
    }
}


void uiSeisSubSel::setInput( const CubeSampling& cs )
{
    if ( is2d_ )
    {
	setInput( cs.hrg );
	setInput( cs.zrg );
    }
    else
    {
	uiBinIDSubSel::Data data = sel3d->getInput();
	data.cs_ = cs;
	sel3d->setInput( data );
    }
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
    return is2d_ ? sel2d->getInput().isall_ : sel3d->getInput().isAll();
}


bool uiSeisSubSel::getSampling( HorSampling& hs ) const
{
    if ( !is2d_ )
	hs = sel3d->getInput().cs_.hrg;
    else
    {
	uiSeis2DSubSel::PosData data = sel2d->getInput();
	hs.start.crl = data.trcrg_.start;
	hs.stop.crl = data.trcrg_.stop;
	hs.step.crl = data.trcrg_.step;
    }
    return true;
}


bool uiSeisSubSel::getZRange( StepInterval<float>& zrg ) const
{
    if ( is2d_ )
	zrg = sel2d->getInput().zrg_;
    else
	zrg = sel3d->getInput().cs_.zrg;
    return true;
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
    return is2d_ ? sel2d->getInput().expectedNrSamples()
		 : sel3d->getInput().expectedNrSamples();
}


int uiSeisSubSel::expectedNrTraces() const
{
    return is2d_ ? sel2d->getInput().expectedNrTraces()
		 : sel3d->getInput().expectedNrTraces();
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


void uiSeis2DSubSel::PosData::clear()
{
    isall_ = true;
    trcrg_.start = 1; trcrg_.stop = mUdf(int); trcrg_.step = 1;
    zrg_ = SI().zRange(true);
}


void uiSeis2DSubSel::PosData::fillPar( IOPar& iopar ) const
{
    iopar.set( sKey::BinIDSel, isall_ ? sKey::No : sKey::Range );

    iopar.set( sKey::FirstInl, 0 );
    iopar.set( sKey::LastInl, mUdf(int) );
    iopar.set( sKey::StepInl, 1 );

    if ( !isall_ )
    {
	iopar.set( sKey::FirstCrl, trcrg_.start );
	iopar.set( sKey::LastCrl, trcrg_.stop );
	iopar.set( sKey::StepCrl, trcrg_.step );

	FileMultiString fms;
	fms += zrg_.start; fms += zrg_.stop; fms += zrg_.step;
	iopar.set( sKey::ZRange, (const char*)fms );
    }
    else
    {
	iopar.set( sKey::FirstCrl, 1 );
	iopar.set( sKey::LastCrl, mUdf(int) );
	iopar.set( sKey::StepCrl, 1 );
    }
}


void uiSeis2DSubSel::PosData::usePar( const IOPar& iopar )
{
    iopar.get( sKey::FirstCrl, trcrg_.start );
    iopar.get( sKey::LastCrl, trcrg_.stop );
    iopar.get( sKey::StepCrl, trcrg_.step );

    const char* res = iopar.find( sKey::ZRange );
    if ( res  )
    {
	FileMultiString fms( res );
	const int sz = fms.size();
	if ( sz > 0 ) zrg_.start = atof( fms[0] );
	if ( sz > 1 ) zrg_.stop = atof( fms[1] );
	if ( sz > 2 ) zrg_.step = atof( fms[2] );
    }

    res = iopar.find( sKey::BinIDSel );
    isall_ = !res || *res != *sKey::Range;
}


int uiSeis2DSubSel::PosData::expectedNrTraces() const
{
    if ( !isall_ )
	return mIsUdf(trcrg_.stop) ? -1 : trcrg_.nrSteps() + 1;
    return -1;
}


int uiSeis2DSubSel::PosData::expectedNrSamples() const
{
    if ( isall_ )
	return SI().zRange(false).nrSteps() + 1;
    else if ( !mIsUdf(zrg_.start) )
	return zrg_.nrSteps() + 1;

    return 0;
}


static const BufferStringSet emptylnms;


uiSeis2DSubSel::uiSeis2DSubSel( uiParent* p, bool for_new_entry, bool mln )
	: uiCompoundParSel(p,"Data subselection")
	, lnmsfld(0)
	, lnmfld(0)
    	, multiln_(mln)
	, lineSel(this)
	, singLineSel(this)
    	, curlnms_(*new BufferStringSet)
{
    butPush.notify( mCB(this,uiSeis2DSubSel,doDlg) );

    uiGenInput* fld;
    if ( for_new_entry )
	fld = lnmfld = new uiGenInput( this, "Store in Set as" );
    else
    {
	fld = lnmsfld = new uiGenInput( this, multiln_ ? "One line only"
				: "Line name", StringListInpSpec(emptylnms) );
	if ( multiln_ )
	{
	    lnmsfld->setWithCheck( true );
	    lnmsfld->checked.notify( mCB(this,uiSeis2DSubSel,singLineChg) );
	}
	lnmsfld->valuechanged.notify( mCB(this,uiSeis2DSubSel,lineChg) );
    }
    fld->attach( alignedBelow, txtfld );

    mainObject()->finaliseDone.notify( mCB(this,uiSeis2DSubSel,updSumm) );
}


uiSeis2DSubSel::~uiSeis2DSubSel()
{
    delete &curlnms_;
}


void uiSeis2DSubSel::clear()
{
    data_.clear();
    updateSummary();

    if ( lnmfld )
	lnmfld->setText( "" );
    else
    {
	if ( multiln_ )
	    lnmsfld->setChecked( false );
	lnmsfld->newSpec( StringListInpSpec(emptylnms), 0 );
    }
}


void uiSeis2DSubSel::setInput( const PosData& data )
{
    data_ = data;
    updateSummary();
}


void uiSeis2DSubSel::setInput( const IOObj& ioobj )
{
    clear();
    if ( !lnmsfld ) return;

    uiSeisIOObjInfo oinf(ioobj,false);
    const BufferString prevlnm( selectedLine() );
    curlnms_.erase();

    oinf.getLineNames( curlnms_ );
    lnmsfld->newSpec( StringListInpSpec(curlnms_), 0 );
    const bool prevok = prevlnm != "" && curlnms_.indexOf(prevlnm) >= 0;
    if ( multiln_ )
	lnmsfld->setChecked( prevok );

    if ( prevok )
	lnmsfld->setText( prevlnm );
}


void uiSeis2DSubSel::doDlg( CallBacker* )
{
    uiSeis2DSubSelDlg dlg( this );
    dlg.setInput( data_ );
    if ( dlg.go() )
	data_ = dlg.data_;
}


BufferString uiSeis2DSubSel::getSummary() const
{
    BufferString txt;
    if ( data_.isall_ )
	txt = "None";
    else
    {
	txt = "Traces "; txt += data_.trcrg_.start;
	txt += "-"; txt += data_.trcrg_.stop;
	txt += " ("; txt += data_.zrg_.nrSteps() + 1; txt += " samples)";
    }
    return txt;
}


void uiSeis2DSubSel::usePar( const IOPar& iopar )
{
    data_.usePar( iopar );
    updateSummary();

    LineKey lk; lk.usePar( iopar, false );
    BufferString lnm( lk.lineName() );
    if ( lnmfld )
	lnmfld->setText( lnm );
    else
    {
	lnmsfld->setText( lnm );
	if ( multiln_ ) lnmsfld->setChecked( lnm != "" );
    }
}


bool uiSeis2DSubSel::fillPar( IOPar& iopar ) const
{
    data_.fillPar( iopar );

    BufferString lnm( selectedLine() );
    if ( lnm == "" )
	iopar.removeWithKey( sKey::LineKey );
    else
	iopar.set( sKey::LineKey, lnm );

    return true;
}


bool uiSeis2DSubSel::isSingLine() const
{
    return lnmfld || !multiln_ || lnmsfld->isChecked();
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


void uiSeis2DSubSel::lineChg( CallBacker* )
{
    lineSel.trigger();
}


void uiSeis2DSubSel::singLineChg( CallBacker* )
{
    singLineSel.trigger();
}


uiSeis2DSubSelDlg::uiSeis2DSubSelDlg( uiParent* p )
    : uiDialog( p, uiDialog::Setup("Data subselection","Select traces/samples",
				   "0.0.0") )
{
    selfld = new uiGenInput( this, "Select", BoolInpSpec("All","Part",true) );
    selfld->valuechanged.notify( mCB(this,uiSeis2DSubSelDlg,selChg) );

    trcrgfld = new uiGenInput( this, "Trace number range",
			       IntInpIntervalSpec(true) );
    trcrgfld->setValue( 1, 0 ); trcrgfld->setValue( 1, 2 );
    trcrgfld->attach( alignedBelow, selfld );

    StepInterval<float> zrg = SI().zRange(true);
    BufferString fldtxt = "Z range "; fldtxt += SI().getZUnit();
    zrgfld = new uiGenInput( this, fldtxt, FloatInpIntervalSpec(true) );
    const float zfac = SI().zFactor();
    zrgfld->setValue( zrg.start*zfac, 0 );
    zrgfld->setValue( zrg.stop*zfac, 1 );
    zrgfld->setValue( zrg.step*zfac, 2 );
    zrgfld->attach( alignedBelow, trcrgfld );

    finaliseDone.notify( mCB(this,uiSeis2DSubSelDlg,selChg) );
}


void uiSeis2DSubSelDlg::setInput( const uiSeis2DSubSel::PosData& data )
{
    data_ = data;
    selfld->setValue( data_.isall_ );
    trcrgfld->setValue( data_.trcrg_ );
    StepInterval<float> zrg = data_.zrg_;
    zrg.scale( SI().zFactor() );
    zrgfld->setValue( zrg );
    selChg();
}


const uiSeis2DSubSel::PosData& uiSeis2DSubSelDlg::getInput() const
{
    data_.isall_ = selfld->getBoolValue();
    if ( !data_.isall_ )
    {
	data_.trcrg_ = trcrgfld->getIStepInterval();
	data_.zrg_ = zrgfld->getFStepInterval();
	data_.zrg_.scale( 1/SI().zFactor() );
    }
    return data_;
}

void uiSeis2DSubSelDlg::selChg( CallBacker* )
{
    data_.isall_ = selfld->getBoolValue();
    trcrgfld->display( !data_.isall_ );
    zrgfld->display( !data_.isall_ );
}


bool uiSeis2DSubSelDlg::acceptOK( CallBacker* )
{
    getInput();
    return true;
}
