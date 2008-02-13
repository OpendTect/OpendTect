/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.cc,v 1.47 2008-02-13 13:28:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseissubsel.h"
#include "uibinidsubsel.h"
#include "uiselsurvranges.h"
#include "uiseisioobjinfo.h"
#include "uigeninput.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "iopar.h"
#include "ioobj.h"
#include "separstr.h"
#include "cubesampling.h"
#include "keystrs.h"
#include "posprovider.h"
#include "uimsg.h"


uiSeisSubSel* uiSeisSubSel::get( uiParent* p, const Seis::SelSetup& s )
{
    if ( s.is2d_ )
       return new uiSeis2DSubSel( p, s );
    else
       return new uiSeis3DSubSel( p, s );
}


void uiSeisSubSel::setInput( const CubeSampling& cs )
{
    setInput( cs.hrg ); setInput( cs.zrg );
}


uiSeis3DSubSel::uiSeis3DSubSel( uiParent* p, const Seis::SelSetup& ss )
    	: uiGroup(p,"3D seis subsel")
{
    uiBinIDSubSel::Setup bss;
    bss.withz(!ss.withoutz_).withstep(ss.withstep_).onlyranges(ss.onlyrange_);

    selfld = new uiBinIDSubSel( this, bss );
    setHAlignObj( selfld );
}


void uiSeis3DSubSel::clear()
{
    selfld->clear();
}


uiCompoundParSel* uiSeis3DSubSel::compoundParSel()
{
    return selfld;
}


void uiSeis3DSubSel::setInput( const HorSampling& hs )
{
    CubeSampling cs = selfld->envelope(); cs.hrg = hs;
    selfld->setInput( cs, false );
}


void uiSeis3DSubSel::setInput( const StepInterval<float>& zrg )
{
    CubeSampling cs = selfld->envelope(); cs.zrg = zrg;
    selfld->setInput( cs, false );
}


void uiSeis3DSubSel::setInput( const CubeSampling& cs )
{
    selfld->setInput( cs, false );
}


void uiSeis3DSubSel::setInput( const IOObj& ioobj )
{
    uiSeisIOObjInfo oinf(ioobj,false); CubeSampling cs;
    if ( !oinf.getRanges(cs) )
	clear();
    else
	setInput( cs );
}


bool uiSeis3DSubSel::isAll() const
{
    return selfld->isAll();
}


void uiSeis3DSubSel::getSampling( HorSampling& hs ) const
{
    if ( selfld->isAll() )
	hs = SI().sampling(false).hrg;
    else 
	hs = selfld->envelope().hrg;
}


void uiSeis3DSubSel::getZRange( StepInterval<float>& zrg ) const
{
    if ( selfld->isAll() )
	zrg = SI().zRange(false);
    else 
	zrg = selfld->envelope().zrg;
}


bool uiSeis3DSubSel::fillPar( IOPar& iop ) const
{
    selfld->fillPar(iop); return true;
}


void uiSeis3DSubSel::usePar( const IOPar& iop )
{
    selfld->usePar( iop );
}


int uiSeis3DSubSel::expectedNrSamples() const
{
    const Pos::Provider* pp = selfld->curProvider();
    if ( !pp ) return SI().zRange(false).nrSteps() + 1;

    return pp->estNrZPerPos();
}


int uiSeis3DSubSel::expectedNrTraces() const
{
    const Pos::Provider* pp = selfld->curProvider();
    if ( !pp ) return SI().zRange(false).nrSteps() + 1;

    return pp->estNrPos();
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


uiSeis2DSubSel::uiSeis2DSubSel( uiParent* p, const Seis::SelSetup& ss )
	: uiCompoundParSel(p,"Data subselection")
	, lnmsfld(0)
	, lnmfld(0)
    	, multiln_(ss.multiline_)
	, lineSel(this)
	, singLineSel(this)
    	, curlnms_(*new BufferStringSet)
{
    butPush.notify( mCB(this,uiSeis2DSubSel,doDlg) );

    uiGenInput* fld;
    if ( ss.fornewentry_ && !multiln_ )
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


void uiSeis2DSubSel::getSampling( HorSampling& hs ) const
{
    hs.start.crl = data_.trcrg_.start;
    hs.stop.crl = data_.trcrg_.stop;
    hs.step.crl = data_.trcrg_.step;
}



void uiSeis2DSubSel::setInput( const PosData& data )
{
    data_ = data;
    updateSummary();
}


void uiSeis2DSubSel::setInput( const StepInterval<float>& zrg )
{
    data_.zrg_ = zrg;
    updateSummary();
}


void uiSeis2DSubSel::setInput( const HorSampling& hs )
{
    data_.trcrg_ = hs.crlRange();
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
    const bool prevok = !prevlnm.isEmpty() && curlnms_.indexOf(prevlnm) >= 0;
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
	if ( multiln_ ) lnmsfld->setChecked( !lnm.isEmpty() );
    }
}


bool uiSeis2DSubSel::fillPar( IOPar& iopar ) const
{
    data_.fillPar( iopar );

    BufferString lnm( selectedLine() );
    if ( lnm.isEmpty() )
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
    selfld = new uiGenInput( this, "Select", BoolInpSpec(true,"All","Part") );
    selfld->valuechanged.notify( mCB(this,uiSeis2DSubSelDlg,selChg) );

    rgfld = new uiSelSubline( this, true );
    rgfld->attach( alignedBelow, selfld );

    finaliseDone.notify( mCB(this,uiSeis2DSubSelDlg,selChg) );
}


void uiSeis2DSubSelDlg::setInput( const uiSeis2DSubSel::PosData& data )
{
    data_ = data;
    selfld->setValue( data_.isall_ );
    rgfld->nrfld_->setRange( data_.trcrg_ );
    rgfld->zfld_->setRange( data_.zrg_ );
    selChg();
}


const uiSeis2DSubSel::PosData& uiSeis2DSubSelDlg::getInput() const
{
    data_.isall_ = selfld->getBoolValue();
    if ( !data_.isall_ )
    {
	data_.trcrg_ = rgfld->nrfld_->getRange();
	data_.zrg_ = rgfld->zfld_->getRange();
    }
    return data_;
}

void uiSeis2DSubSelDlg::selChg( CallBacker* )
{
    data_.isall_ = selfld->getBoolValue();
    rgfld->display( !data_.isall_ );
}


bool uiSeis2DSubSelDlg::acceptOK( CallBacker* )
{
    getInput();
    return true;
}
