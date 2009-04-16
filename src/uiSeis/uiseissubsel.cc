/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseissubsel.cc,v 1.62 2009-04-16 14:45:05 cvsbert Exp $";

#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uicompoundparsel.h"
#include "uipossubsel.h"
#include "uiposprovider.h"
#include "uigeninput.h"
#include "uilistbox.h"
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
    selfld_->setInputLimit( cs );
}


void uiSeisSubSel::setInput( const StepInterval<float>& zrg )
{
    CubeSampling cs = selfld_->envelope(); cs.zrg = zrg;
    selfld_->setInputLimit( cs );
}


void uiSeisSubSel::setInput( const CubeSampling& cs )
{
    selfld_->setInputLimit( cs );
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
    if ( !pp ) return SI().sampling(false).hrg.totalNr();

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
	selfld_->setInputLimit( cs );
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
    if ( ss.fornewentry_ && !multiln_ )
    {
	selfld_->display( false );
	lnmfld_ = new uiSeis2DLineNameSel( this, false );
	setHAlignObj( lnmfld_ );
    }
    else
    {
	lnmsfld_ = new uiGenInput( this, multiln_ ? "One line only"
				: "Line name", StringListInpSpec(emptylnms) );
	if ( multiln_ )
	{
	    lnmsfld_->setWithCheck( true );
	    lnmsfld_->checked.notify( mCB(this,uiSeis2DSubSel,singLineChg) );
	    lnmsfld_->attach( alignedBelow, selfld_ );
	}
	else
	    selfld_->attach( alignedBelow, lnmsfld_ );

	lnmsfld_->valuechanged.notify( mCB(this,uiSeis2DSubSel,lineChg) );
    }
}


uiSeis2DSubSel::~uiSeis2DSubSel()
{
    delete &curlnms_;
}


void uiSeis2DSubSel::clear()
{
    uiSeisSubSel::clear();

    if ( lnmfld_ )
	lnmfld_->setInput( "" );
    else
    {
	if ( multiln_ )
	    lnmsfld_->setChecked( false );
	lnmsfld_->newSpec( StringListInpSpec(emptylnms), 0 );
    }

    trcrgs_.erase();
    zrgs_.erase();
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


void uiSeis2DSubSel::setInputWithAttrib( const IOObj& ioobj,
					 const char* attribnm )
{
    clear();
    if ( !lnmsfld_ ) return;

    SeisIOObjInfo info( ioobj );
    const BufferString prevlnm( selectedLine() );
    curlnms_.erase();

    info.getLineNamesWithAttrib( attribnm, curlnms_ );
    for ( int idx=0; idx<curlnms_.size(); idx++ )
    {
	LineKey lk( curlnms_.get(idx), attribnm );
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	if ( !info.getRanges(lk,trcrg,zrg) )
	    break;

	trcrgs_ += trcrg;
	zrgs_ += zrg;
    }

    lnmsfld_->newSpec( StringListInpSpec(curlnms_), 0 );
    const bool prevok = !prevlnm.isEmpty() && curlnms_.indexOf(prevlnm) >= 0;
    if ( multiln_ )
	lnmsfld_->setChecked( prevok );

    if ( prevok )
	lnmsfld_->setText( prevlnm );

    lineChg( 0 );
}


void uiSeis2DSubSel::usePar( const IOPar& iopar )
{
    uiSeisSubSel::usePar( iopar );

    LineKey lk; lk.usePar( iopar, false );
    BufferString lnm( lk.lineName() );
    if ( lnmfld_ )
	lnmfld_->setInput( lnm );
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
    {
	if ( lnmfld_ )
	    { uiMSG().error("Please enter a line name"); return false; }

	iopar.removeWithKey( sKey::LineKey );
    }
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
    if ( !isSingLine() ) return "";

    return lnmfld_ ? lnmfld_->getInput() : lnmsfld_->text();
}


void uiSeis2DSubSel::setSelectedLine( const char* nm )
{
    if ( lnmfld_ )
	lnmfld_->setInput( nm );
    else
	lnmsfld_->setText( nm );
}


void uiSeis2DSubSel::lineChg( CallBacker* )
{
    if ( isSingLine() )
    {
	const int lidx = lnmsfld_->getIntValue();
	if ( lidx >= 0 && lidx < trcrgs_.size() && lidx < zrgs_.size() )
	{
	    CubeSampling cs;
	    StepInterval<int> inlrg( 0, 0, 1 );
	    cs.hrg.set( inlrg, trcrgs_[lidx] );
	    cs.zrg = zrgs_[lidx];
	    selfld_->provSel()->setInputLimit( cs );
	}
    }

    lineSel.trigger();
}


void uiSeis2DSubSel::singLineChg( CallBacker* )
{
    singLineSel.trigger();
}
