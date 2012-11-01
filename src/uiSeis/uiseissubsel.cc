/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          June 2004
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uicompoundparsel.h"
#include "uipossubsel.h"
#include "uiposprovider.h"
#include "uibutton.h"
#include "uilistbox.h"
#include "uiseisioobjinfo.h"
#include "uimsg.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "survinfo.h"
#include "iopar.h"
#include "ioobj.h"
#include "ioman.h"
#include "cubesampling.h"
#include "keystrs.h"
#include "posprovider.h"
#include "zdomain.h"


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
				  : uiPosSubSel::Setup::OnlySeisTypes)
    	.zdomkey(ss.zdomkey_);

    selfld_ = new uiPosSubSel( this, pss );
    setHAlignObj( selfld_ );
}


bool uiSeisSubSel::isAll() const
{
    return selfld_->isAll();
}


void uiSeisSubSel::getSampling( CubeSampling& cs ) const
{
    cs = selfld_->envelope();
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


void uiSeisSubSel::setInput( const MultiID& id )
{
    IOObj* ioobj = IOM().get( id );
    if ( ioobj )
	setInput( *ioobj );
    delete ioobj;
}


void uiSeisSubSel::setInput( const HorSampling& hs )
{
    CubeSampling cs = selfld_->envelope(); cs.hrg = hs;
    selfld_->setInput( cs );
}


void uiSeisSubSel::setInput( const StepInterval<float>& zrg )
{
    CubeSampling cs = selfld_->envelope(); cs.zrg = zrg;
    selfld_->setInput( cs );
}


void uiSeisSubSel::setInput( const CubeSampling& cs )
{
    selfld_->setInput( cs );
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
    mDynamicCastGet( const uiSeis2DSubSel*, ss2d, this )                              
    if ( !pp ) return ss2d ? 0 : mCast(int, SI().sampling(false).hrg.totalNr());

    return mCast( int, pp->estNrPos() );
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
    {
	selfld_->setInput( cs );
	if ( &oinf.zDomainDef() != &ZDomain::SI() )
	{
	    CubeSampling limcs( selfld_->inputLimit() );
	    limcs.zrg.start = -1e9; limcs.zrg.stop = 1e9;
	    limcs.zrg.step = 0.001;
	    selfld_->setInputLimit( limcs );
	}
    }
}


uiSeis2DSubSel::uiSeis2DSubSel( uiParent* p, const Seis::SelSetup& ss )
	: uiSeisSubSel(p,ss)
	, onelnbox_(0)
    	, multiln_(ss.multiline_)
	, lineSel(this)
	, singLineSel(this)
    	, curlnms_(*new BufferStringSet)
{
    lnmfld_ = new uiSeis2DLineNameSel( this, !ss.fornewentry_ );
    setHAlignObj( lnmfld_ );

    if ( ss.fornewentry_ && multiln_ )
	selfld_->display( false );
    else
    {
	if ( multiln_ )
	{
	    onelnbox_ = new uiCheckBox( this, "Single line:" );
	    onelnbox_->activated.notify( mCB(this,uiSeis2DSubSel,singLineChg) );
	    onelnbox_->attach( leftOf, lnmfld_ );
	    lnmfld_->attach( alignedBelow, selfld_ );
	    lnmfld_->setSensitive( false );
	}
	else
	    selfld_->attach( alignedBelow, lnmfld_ );

	lnmfld_->nameChanged.notify( mCB(this,uiSeis2DSubSel,lineChg) );
    }
}


uiSeis2DSubSel::~uiSeis2DSubSel()
{
    delete &curlnms_;
}


void uiSeis2DSubSel::clear()
{
    uiSeisSubSel::clear();

    lnmfld_->setInput( "" );
    if ( multiln_ )
	onelnbox_->setChecked( false );

    trcrgs_.erase();
    zrgs_.erase();
}


void uiSeis2DSubSel::setInput( const IOObj& ioobj )
{
    clear();
    lnmfld_->setLineSet( ioobj.key() );
}


void uiSeis2DSubSel::setInputWithAttrib( const IOObj& ioobj,
					 const char* attribnm )
{
    setInput( ioobj );

    SeisIOObjInfo info( ioobj );
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
}


void uiSeis2DSubSel::usePar( const IOPar& iopar )
{
    uiSeisSubSel::usePar( iopar );

    LineKey lk; lk.usePar( iopar, false );
    BufferString lnm( lk.lineName() );
    lnmfld_->setInput( lnm );
    if ( multiln_ ) onelnbox_->setChecked( !lnm.isEmpty() );
}


bool uiSeis2DSubSel::fillPar( IOPar& iopar ) const
{
    if ( !uiSeisSubSel::fillPar(iopar) )
	return false;

    BufferString lnm( selectedLine() );
    if ( lnm.isEmpty() )
    {
	if ( !multiln_ )
	    { uiMSG().error("Please enter a line name"); return false; }

	iopar.removeWithKey( sKey::LineKey() );
    }
    else
	iopar.set( sKey::LineKey(), lnm );

    return true;
}


bool uiSeis2DSubSel::isSingLine() const
{
    return !multiln_ || onelnbox_->isChecked();
}


const char* uiSeis2DSubSel::selectedLine() const
{
    return isSingLine() ? lnmfld_->getInput() : "";
}


void uiSeis2DSubSel::setSelectedLine( const char* nm )
{
    lnmfld_->setInput( nm );
}


void uiSeis2DSubSel::lineChg( CallBacker* )
{
    if ( isSingLine() )
    {
	const int lidx = lnmfld_->getLineIndex();
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
    lnmfld_->setSensitive( onelnbox_->isChecked() );
    singLineSel.trigger();
}
