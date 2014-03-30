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
    	, multiln_(ss.multiline_)
	, lineSel(this)
{
    lnmfld_ = new uiSeis2DLineSel( this, multiln_ );
    setHAlignObj( lnmfld_ );

    if ( ss.fornewentry_ && multiln_ )
	selfld_->display( false );
    else
	selfld_->attach( alignedBelow, lnmfld_ );
	
    lnmfld_->selectionChanged.notify( mCB(this,uiSeis2DSubSel,lineChg) );
}


uiSeis2DSubSel::~uiSeis2DSubSel()
{
}


void uiSeis2DSubSel::clear()
{
    uiSeisSubSel::clear();

    lnmfld_->clearSelection();
}


void uiSeis2DSubSel::setInput( const IOObj& ioobj )
{
    clear();
    lnmfld_->setInput( ioobj.key() );
}


void uiSeis2DSubSel::usePar( const IOPar& iopar )
{
    uiSeisSubSel::usePar( iopar );

    BufferStringSet lnms;
    iopar.get( sKey::LineKey(), lnms );
    lnmfld_->setSelLineNames( lnms );
}


bool uiSeis2DSubSel::fillPar( IOPar& iopar ) const
{
    if ( !uiSeisSubSel::fillPar(iopar) )
	return false;

    BufferStringSet sellnms;
    lnmfld_->getSelLineNames( sellnms );
    if ( sellnms.isEmpty() )
    {
	BufferString msg( "Please select " );
	msg.add( multiln_ ? "at least one line" : "the line" );
	uiMSG().error( msg );
	return false;
    }
    
    iopar.set( sKey::LineKey(), sellnms );
    return true;
}


bool uiSeis2DSubSel::isSingLine() const
{
    if ( !multiln_ )
	return true;

    BufferStringSet sellnms;
    lnmfld_->getSelLineNames( sellnms );
    return sellnms.size() == 1;
}


const char* uiSeis2DSubSel::selectedLine() const
{
    return isSingLine() ? lnmfld_->lineName() : "";
}


void uiSeis2DSubSel::setSelectedLine( const char* nm )
{
    lnmfld_->setSelLine( nm );
}


void uiSeis2DSubSel::lineChg( CallBacker* )
{
    if ( isSingLine() )
    {
	const Pos::GeomID selid = lnmfld_->geomID();
	SeisIOObjInfo oif( inpkey_ );
	StepInterval<float> zrg;
	StepInterval<int> trcrg;
	if ( oif.getRanges(selid,trcrg,zrg) )
	{
	    CubeSampling cs;
	    StepInterval<int> inlrg( 0, 0, 1 );
	    cs.hrg.set( inlrg, trcrg );
	    cs.zrg = zrg;
	    selfld_->provSel()->setInputLimit( cs );
	}
    }

    lineSel.trigger();
}

