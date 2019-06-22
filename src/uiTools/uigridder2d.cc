/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2008
________________________________________________________________________

-*/

#include "uigridder2d.h"

#include "gridder2d.h"
#include "keystrs.h"
#include "uigeninput.h"
#include "survinfo.h"

mImplClassFactory( uiGridder2DGrp, factory );


uiGridder2DSel::uiGridder2DSel( uiParent* p, const Gridder2D* g,
				PolyTrend::Order trend )
    : uiDlgGroup( p, uiStrings::sGridding() )
{
    int selidx = mUdf(int);
    const BufferStringSet griddernames = Gridder2D::factory().getKeys();
    const uiStringSet gridderusernames = Gridder2D::factory().getUserNames();
    uiStringSet gridderusedusernames;

    for ( int idx=0; idx<griddernames.size(); idx++ )
    {
	const BufferString& griddernm = griddernames.get( idx );
	uiGridder2DGrp* uigriddergrp
		= uiGridder2DGrp::factory().createSuitable( this, griddernm );
	if ( !uigriddergrp )
	    continue;

	if ( g && BufferString(g->factoryKeyword()) == griddernm )
	{
	    selidx = griddinggrps_.size();
	    IOPar gridpar;
	    gridpar.set( sKey::Name(), griddernm );
	    g->fillPar( gridpar );
	    gridpar.set( PolyTrend::sKeyOrder(),
			 PolyTrend::OrderDef().getKey(trend) );
	    uigriddergrp->usePar( gridpar );
	}

	griddinggrps_ += uigriddergrp;
	gridderusedusernames.add( gridderusernames.get(idx) );
	uigriddergrp->display( false );
    }

    griddingsel_ = new uiGenInput( this, uiStrings::sAlgorithm(),
				   StringListInpSpec( gridderusedusernames ) );
    griddingsel_->valuechanged.notify( mCB(this,uiGridder2DSel,selChangeCB) );

    for ( int idx=0; idx<griddinggrps_.size(); idx++ )
	griddinggrps_[idx]->attach( alignedBelow, griddingsel_ );

    if ( mIsUdf(selidx) )
	selidx = 0;

    griddingsel_->setValue( selidx );
    setHAlignObj( griddingsel_ );
    selChangeCB( 0 );
}


uiGridder2DSel::~uiGridder2DSel()
{
    detachAllNotifiers();
}


bool uiGridder2DSel::usePar( const IOPar& par )
{
    uiGridder2DGrp* griddergrp = const_cast<uiGridder2DGrp*>( getSel() );

    return griddergrp ? griddergrp->usePar(par) : false;
}


void uiGridder2DSel::fillPar( IOPar& par, bool withprefix ) const
{
    const uiGridder2DGrp* griddergrp = getSel();
    IOPar gridderpar;
    if ( griddergrp )
    {
	if ( !griddergrp->fillPar(gridderpar) ||
	     !griddergrp->errMsg().isEmpty() )
	    msg_ = griddergrp->errMsg();
    }

    if ( withprefix )
	par.mergeComp( gridderpar, Gridder2D::sKeyGridder() );
    else
	par.merge( gridderpar );
}


const uiGridder2DGrp* uiGridder2DSel::getSel() const
{
    const int selidx = griddingsel_->getIntValue();
    if ( !griddinggrps_.validIdx(selidx) || !griddinggrps_[selidx] )
	return 0;

    return griddinggrps_[selidx];
}


void uiGridder2DSel::selChangeCB( CallBacker* )
{
    for ( int idx=griddinggrps_.size()-1; idx>=0; idx-- )
	if ( griddinggrps_[idx] ) griddinggrps_[idx]->display( false );

    const int selidx = griddingsel_->getIntValue();

    if ( griddinggrps_.validIdx(selidx) && griddinggrps_[selidx] )
	griddinggrps_[selidx]->display( true );
}


uiGridder2DGrp::uiGridder2DGrp( uiParent* p, const uiString& nm,
				const BufferString& griddernm, bool withtrend )
    : uiDlgGroup(p,nm)
    , gridder_(Gridder2D::factory().create(griddernm.buf()))
    , trendfld_(0)
{
    fillPar( initialstate_ );

    if ( !withtrend )
	return;

    initialstate_.set( PolyTrend::sKeyOrder(),
		       PolyTrend::OrderDef().getKey(PolyTrend::Order0) );

    trendfld_ = new uiGenInput( this, tr("Trend Polynomial"),
				StringListInpSpec(PolyTrend::OrderDef()) );
    setHAlignObj( trendfld_ );
    rejectOK();
}


uiGridder2DGrp::~uiGridder2DGrp()
{
    delete gridder_;
}


bool uiGridder2DGrp::usePar( const IOPar& par )
{
    if ( trendfld_ )
    {
	PolyTrend::Order trend;
	if ( PolyTrend::OrderDef().parse(par,PolyTrend::sKeyOrder(),trend) )
	    trendfld_->setValue( PolyTrend::OrderDef().indexOf( trend ) );
    }

    BufferString griddernm;
    if ( !par.get(sKey::Name(),griddernm) && !gridder_ )
	return false;

    if ( !gridder_ ||
	 ( griddernm.isEmpty() &&
	   BufferString(gridder_->factoryKeyword()) != griddernm ) )
    {
	delete gridder_;
	gridder_ = Gridder2D::factory().create( griddernm.buf() );
    }

    if ( !gridder_ || !gridder_->usePar(par) )
	return false;

    putToScreen();

    return true;
}


bool uiGridder2DGrp::fillPar( IOPar& par ) const
{
    if ( !gridder_ )
	return false;

    par.set( sKey::Name(), gridder_->factoryKeyword() );
    getFromScreen();
    gridder_->fillPar( par );
    if ( !trendfld_ )
	return true;

    const PolyTrend::Order trend =
		PolyTrend::OrderDef().getEnumForIndex(trendfld_->getIntValue());
    par.set( PolyTrend::sKeyOrder(), PolyTrend::OrderDef().getKey(trend) );

    return true;
}


bool uiGridder2DGrp::revertChanges()
{
    return usePar( initialstate_ );
}



void uiInverseDistanceGridder2D::initClass()
{
    factory().addCreator( uiInverseDistanceGridder2D::create,
			  InverseDistanceGridder2D::sFactoryKeyword() );
}


uiGridder2DGrp* uiInverseDistanceGridder2D::create( uiParent* p,
						const BufferString& griddernm )
{
    if ( griddernm != InverseDistanceGridder2D::sFactoryKeyword() )
	return 0;

    return new uiInverseDistanceGridder2D( p, griddernm );
}


uiInverseDistanceGridder2D::uiInverseDistanceGridder2D ( uiParent* p,
						const BufferString& griddernm )
    : uiGridder2DGrp(p,InverseDistanceGridder2D::sFactoryDisplayName(),
		     griddernm)
{
    msg_ = InverseDistanceGridder2D::searchRadiusErrMsg();
    //the only thing that can go wrong

    const uiString radstr = uiStrings::sSearchRadius().withSurvXYUnit();
    searchradiusfld_ = new uiGenInput( this, radstr, FloatInpSpec() );
    searchradiusfld_->setWithCheck( true );
    if ( trendFld() )
	trendFld()->attach( alignedBelow, searchradiusfld_ );

    setHAlignObj( searchradiusfld_ );

    revertChanges();
}


void uiInverseDistanceGridder2D::getFromScreen() const
{
    IOPar par;
    const float radius = searchradiusfld_->isChecked()
		       ? searchradiusfld_->getFValue() : mUdf(float);
    par.set( InverseDistanceGridder2D::sKeySearchRadius(), radius );
    gridder_->usePar( par );
}


void uiInverseDistanceGridder2D::putToScreen()
{
    IOPar par;
    gridder_->fillPar( par );
    float radius = mUdf(float);
    if ( par.get(InverseDistanceGridder2D::sKeySearchRadius(),radius) )
	searchradiusfld_->setValue( radius );

    searchradiusfld_->setChecked( !mIsUdf(radius) );
}


void uiTriangulatedGridder2D::initClass()
{
    factory().addCreator( uiTriangulatedGridder2D::create,
			  TriangulatedGridder2D::sFactoryKeyword() );
}


uiGridder2DGrp* uiTriangulatedGridder2D::create( uiParent* p,
						const BufferString& griddernm )
{
    if ( griddernm != TriangulatedGridder2D::sFactoryKeyword() )
	return 0;

    return new uiTriangulatedGridder2D( p, griddernm );
}


uiTriangulatedGridder2D::uiTriangulatedGridder2D ( uiParent* p,
						 const BufferString& griddernm )
    : uiGridder2DGrp( p, TriangulatedGridder2D::sFactoryDisplayName(),
		      griddernm )
{
    revertChanges();
}
