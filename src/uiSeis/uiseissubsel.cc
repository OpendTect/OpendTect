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
	, multilnmsel_(0)
	, singlelnmsel_(0)
{
    if ( multiln_ )
    {
	multilnmsel_ = new uiSeis2DMultiLineSel( this, 0, !ss.withoutz_,
						 ss.withstep_ );
	setHAlignObj( multilnmsel_ );
	multilnmsel_->selectionChanged.notify(mCB(this,uiSeis2DSubSel,lineChg));
    }
    else
    {
	singlelnmsel_ = new uiSeis2DLineNameSel( this, !ss.fornewentry_ );
	setHAlignObj( singlelnmsel_ );
	singlelnmsel_->nameChanged.notify( mCB(this,uiSeis2DSubSel,lineChg) );
    }

    if ( multilnmsel_ )
	selfld_->display( false );
    else
	selfld_->attach( alignedBelow, singlelnmsel_ );

}


uiSeis2DSubSel::~uiSeis2DSubSel()
{
}


void uiSeis2DSubSel::clear()
{
    uiSeisSubSel::clear();

    if ( multilnmsel_ )
	multilnmsel_->clearSelection();
}


void uiSeis2DSubSel::setInput( const IOObj& ioobj )
{
    clear();
    inpkey_ = ioobj.key();
    if ( multilnmsel_ )
	multilnmsel_->setInput( inpkey_ );
    else
	singlelnmsel_->setDataSet( inpkey_ );
}


void uiSeis2DSubSel::usePar( const IOPar& iopar )
{
    BufferStringSet lnms;
    iopar.get( sKey::LineKey(), lnms );
    if ( lnms.isEmpty() && multilnmsel_ )
    {
	multilnmsel_->usePar( iopar );
	return;
    }

    if ( multilnmsel_ )
	multilnmsel_->setSelLineNames( lnms );
    else if ( !lnms.isEmpty() )
	singlelnmsel_->setInput( lnms.get(0).buf() );
}


bool uiSeis2DSubSel::fillPar( IOPar& iopar ) const
{
    if ( multilnmsel_ )
    {
	if ( !multilnmsel_->nrSelected() )
	{
	    uiMSG().error( "Please select at least one line" );
	    return false;
	}

	return multilnmsel_->fillPar( iopar );
    }

    const FixedString sellinenm( singlelnmsel_->getInput() );
    if ( sellinenm.isEmpty() )
    {
	uiMSG().error( "Please select the line" );
	return false;
    }

    iopar.set( sKey::LineKey(), sellinenm );
    return true;
}


bool uiSeis2DSubSel::isSingLine() const
{
    return !multilnmsel_ || (multilnmsel_->nrSelected() == 1);
}


const char* uiSeis2DSubSel::selectedLine() const
{
    return multilnmsel_ ? multilnmsel_->lineName() : singlelnmsel_->getInput();
}


void uiSeis2DSubSel::setSelectedLine( const char* nm )
{
    if ( multilnmsel_ )
	multilnmsel_->setSelLine( nm );
    else
	singlelnmsel_->setInput( nm );
}


void uiSeis2DSubSel::selectedGeomIDs( TypeSet<Pos::GeomID>& geomids ) const
{
    geomids.erase();
    if ( multilnmsel_ )
	multilnmsel_->getSelGeomIDs( geomids );
    else
	geomids += singlelnmsel_->getInputGeomID();
}


void uiSeis2DSubSel::selectedLines( BufferStringSet& lnms ) const
{
    lnms.erase();
    if ( multilnmsel_ )
	multilnmsel_->getSelLineNames( lnms );
    else
	lnms.add( singlelnmsel_->getInput() );
}


void uiSeis2DSubSel::setSelectedLines( const BufferStringSet& lnms )
{
    if ( multilnmsel_ )
	multilnmsel_->setSelLineNames( lnms );
    else if ( !lnms.isEmpty() )
	singlelnmsel_->setInput( lnms.get(0) );
}


void uiSeis2DSubSel::lineChg( CallBacker* )
{
    if ( singlelnmsel_ )
    {
	const Pos::GeomID selid = singlelnmsel_->getInputGeomID();
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

