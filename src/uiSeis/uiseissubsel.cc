/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
 RCS:           $Id: uiseissubsel.cc,v 1.48 2008-02-18 11:00:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseissubsel.h"
#include "uipossubsel.h"
#include "uiposprovider.h"
#include "uigeninput.h"
#include "uiseisioobjinfo.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "iopar.h"
#include "ioobj.h"
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


uiSeisSubSel::uiSeisSubSel( uiParent* p, const Seis::SelSetup& ss )
    	: uiGroup(p,"Seis subsel")
{
    uiPosSubSel::Setup pss( ss.is2d_, !ss.withoutz_ );
    pss.withstep(ss.withstep_)
	.choicetype(ss.onlyrange_ ? uiPosSubSel::Setup::OnlyRanges
				  : uiPosSubSel::Setup::OnlySeisTypes);

    selfld_ = new uiPosSubSel( this, pss );
    setHAlignObj( selfld_ );
}


bool uiSeisSubSel::isAll() const
{
    return selfld_->isAll();
}


void uiSeisSubSel::getSampling( HorSampling& hs ) const
{
    hs = selfld_->envelope().hrg;
}


void uiSeisSubSel::getZRange( StepInterval<float>& zrg ) const
{
    zrg = selfld_->envelope().zrg;
}


bool uiSeisSubSel::fillPar( IOPar& iop ) const
{
    selfld_->fillPar(iop); return true;
}


void uiSeisSubSel::usePar( const IOPar& iop )
{
    selfld_->usePar( iop );
}


void uiSeisSubSel::clear()
{
    selfld_->clear();
}


void uiSeisSubSel::setInput( const HorSampling& hs )
{
    CubeSampling cs = selfld_->envelope(); cs.hrg = hs;
    selfld_->setInput( cs, false );
}


void uiSeisSubSel::setInput( const StepInterval<float>& zrg )
{
    CubeSampling cs = selfld_->envelope(); cs.zrg = zrg;
    selfld_->setInput( cs, false );
}


void uiSeisSubSel::setInput( const CubeSampling& cs )
{
    selfld_->setInput( cs, false );
}


int uiSeisSubSel::expectedNrSamples() const
{
    const Pos::Provider* pp = selfld_->curProvider();
    if ( !pp ) return SI().zRange(false).nrSteps() + 1;

    return pp->estNrZPerPos();
}


int uiSeisSubSel::expectedNrTraces() const
{
    const Pos::Provider* pp = selfld_->curProvider();
    if ( !pp ) return SI().sampling(false).totalNr();

    return pp->estNrPos();
}


uiCompoundParSel* uiSeisSubSel::compoundParSel()
{
    return selfld_->provSel();
}


void uiSeis3DSubSel::setInput( const IOObj& ioobj )
{
    uiSeisIOObjInfo oinf(ioobj,false); CubeSampling cs;
    if ( !oinf.getRanges(cs) )
	clear();
    else
	selfld_->setInput( cs, false );
}


static const BufferStringSet emptylnms;


uiSeis2DSubSel::uiSeis2DSubSel( uiParent* p, const Seis::SelSetup& ss )
	: uiSeisSubSel(p,ss)
	, lnmsfld_(0)
	, lnmfld_(0)
    	, multiln_(ss.multiline_)
	, lineSel(this)
	, singLineSel(this)
    	, curlnms_(*new BufferStringSet)
{
    uiGenInput* fld;
    if ( ss.fornewentry_ && !multiln_ )
	fld = lnmfld_ = new uiGenInput( this, "Store in Set as" );
    else
    {
	fld = lnmsfld_ = new uiGenInput( this, multiln_ ? "One line only"
				: "Line name", StringListInpSpec(emptylnms) );
	if ( multiln_ )
	{
	    lnmsfld_->setWithCheck( true );
	    lnmsfld_->checked.notify( mCB(this,uiSeis2DSubSel,singLineChg) );
	}
	lnmsfld_->valuechanged.notify( mCB(this,uiSeis2DSubSel,lineChg) );
    }
    fld->attach( alignedBelow, selfld_ );
}


uiSeis2DSubSel::~uiSeis2DSubSel()
{
    delete &curlnms_;
}


void uiSeis2DSubSel::clear()
{
    uiSeisSubSel::clear();

    if ( lnmfld_ )
	lnmfld_->setText( "" );
    else
    {
	if ( multiln_ )
	    lnmsfld_->setChecked( false );
	lnmsfld_->newSpec( StringListInpSpec(emptylnms), 0 );
    }
}


void uiSeis2DSubSel::setInput( const IOObj& ioobj )
{
    clear();
    if ( !lnmsfld_ ) return;

    uiSeisIOObjInfo oinf(ioobj,false);
    const BufferString prevlnm( selectedLine() );
    curlnms_.erase();

    oinf.getLineNames( curlnms_ );
    lnmsfld_->newSpec( StringListInpSpec(curlnms_), 0 );
    const bool prevok = !prevlnm.isEmpty() && curlnms_.indexOf(prevlnm) >= 0;
    if ( multiln_ )
	lnmsfld_->setChecked( prevok );

    if ( prevok )
	lnmsfld_->setText( prevlnm );
}


void uiSeis2DSubSel::usePar( const IOPar& iopar )
{
    uiSeisSubSel::usePar( iopar );

    LineKey lk; lk.usePar( iopar, false );
    BufferString lnm( lk.lineName() );
    if ( lnmfld_ )
	lnmfld_->setText( lnm );
    else
    {
	lnmsfld_->setText( lnm );
	if ( multiln_ ) lnmsfld_->setChecked( !lnm.isEmpty() );
    }
}


bool uiSeis2DSubSel::fillPar( IOPar& iopar ) const
{
    if ( !uiSeisSubSel::fillPar(iopar) )
	return false;

    BufferString lnm( selectedLine() );
    if ( lnm.isEmpty() )
	iopar.removeWithKey( sKey::LineKey );
    else
	iopar.set( sKey::LineKey, lnm );

    return true;
}


bool uiSeis2DSubSel::isSingLine() const
{
    return lnmfld_ || !multiln_ || lnmsfld_->isChecked();
}


const char* uiSeis2DSubSel::selectedLine() const
{
    return isSingLine() ? (lnmfld_ ? lnmfld_ : lnmsfld_)->text() : "";
}


void uiSeis2DSubSel::setSelectedLine( const char* nm )
{
    if ( lnmfld_ )
	lnmfld_->setText( nm );
    else
	lnmsfld_->setText( nm );
}


void uiSeis2DSubSel::lineChg( CallBacker* )
{
    lineSel.trigger();
}


void uiSeis2DSubSel::singLineChg( CallBacker* )
{
    singLineSel.trigger();
}
