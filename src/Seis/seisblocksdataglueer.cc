/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocksdataglueer.h"
#include "seisstorer.h"
#include "arrayndimpl.h"
#include "seistrc.h"
#include "survinfo.h"
#include "uistrings.h"


namespace Seis
{
namespace Blocks
{

mUseType( DataGlueer, z_type );
mUseType( DataGlueer, val_type );
mUseType( DataGlueer, pos_type );
mUseType( DataGlueer, Arr2D );
mUseType( DataGlueer, Arr3D );
typedef ArrayND<z_type> ArrND;

class Blocklet
{
public:


    Blocklet( const Arr2D& inp, pos_type tnr, pos_type tstep,
				z_type z, z_type zstep )
	: arr_(new Array2DImpl<val_type>(inp))
	, tnrsd_(tnr,tstep), zsd_(z,zstep)	    {}
    Blocklet( const Arr3D& inp, pos_type tnr, pos_type tstep,
				z_type z, z_type zstep )
	: arr_(new Array3DImpl<val_type>(inp))
	, tnrsd_(tnr,tstep), zsd_(z,zstep)	    {}
    ~Blocklet()	{ delete arr_; }

    ArrND*			arr_;
    SamplingData<pos_type>	tnrsd_;
    SamplingData<z_type>	zsd_;

    bool	is3D() const		{ return arr_->nrDims() > 2; }
    int		nrTrcs() const	{ return arr_->getSize(arr_->nrDims()-2); }
    pos_type	trcNrStart() const	{ return tnrsd_.start; }
    pos_type	trcNrStep() const	{ return tnrsd_.step; }
    pos_type	trcNrStop() const	{ return tnrsd_.atIndex( nrTrcs()-1 ); }
    int		nrZ() const	{ return arr_->getSize(arr_->nrDims()-1); }
    z_type	zStart() const		{ return zsd_.start; }
    z_type	zStep() const		{ return zsd_.step; }
    z_type	zStop() const		{ return zsd_.atIndex( nrZ() - 1 ); }

void addData( const StepInterval<pos_type>& lnrs, SeisTrc& trc,
	      Array1D<float>& contrib )
{
    const auto lnr = trc.info().lineNr();
    const auto tnr = trc.info().trcNr();
    if ( lnr < lnrs.start || lnr > lnrs.stop
      || tnr < tnrsd_.start || tnr > trcNrStop() )
	return;

    const auto nrz = nrZ();
    const auto trczidx0 = trc.info().sampling_.nearestIndex( zStart() );
    if ( is3D() )
    {
	const auto idx0 = lnrs.nearestIndex( lnr );
	const auto idx1 = tnrsd_.nearestIndex( tnr );
	mDynamicCastGet( Arr3D*, arr3d, arr_ )
	for ( auto zidx=0; zidx<nrz; zidx++ )
	{
	    const auto idx2 = trczidx0 + zidx;
	    const auto prevwt = contrib[idx2];
	    const auto prevval = trc.get( idx2, 0 );
	    const auto newwt = 1.0f;
	    const auto newval = arr3d->get( idx0, idx1, zidx );

	    const auto nextwt = prevwt + newwt;
	    const auto wtavg = (prevwt * prevval + newwt * newval) / nextwt;
	    trc.set( idx2, wtavg, 0 );
	    contrib.set( idx2, nextwt );
	}
    }
    else
    {
	// TODO similar, copy&adapt when 3D works fine
    }
}

};


class LineBuf
{
public:

    mUseType( DataGlueer,	Arr2D );
    mUseType( DataGlueer,	Arr3D );
    mUseType( DataGlueer,	idx_type );

			LineBuf( pos_type startlnr, pos_type lstep=1 )
			    : startlnr_(startlnr)
			    , linerg_(startlnr,startlnr,lstep)	{}
			~LineBuf()		{ deepErase(blocklets_); }

    const pos_type	startlnr_;
    ObjectSet<Blocklet>	blocklets_;
    StepInterval<pos_type> linerg_;


pos_type lineNr4Idx( int lidx ) const
{
    return startlnr_ + linerg_.step * lidx;
}


pos_type lastLineNr( const Blocklet& bll ) const
{
    return bll.is3D() ? lineNr4Idx( bll.arr_->getSize(0)-1 ) : startlnr_;
}


void add( Blocklet* bl )
{
    blocklets_ += bl;
    linerg_.include( lastLineNr(*bl) );
}


StepInterval<pos_type> trcNrRange() const
{
    const auto& bll0 = *blocklets_.first();
    StepInterval<pos_type> rg( bll0.trcNrStart(), bll0.trcNrStop(),
			       bll0.trcNrStep() );
    for ( auto* bll : blocklets_ )
    {
	rg.include( bll->trcNrStart(), false );
	rg.include( bll->trcNrStop(), false );
    }
    return rg;
}


StepInterval<z_type> zRange() const
{
    const auto& bll0 = *blocklets_.first();
    StepInterval<z_type> zrg( bll0.zStart(), bll0.zStop(), bll0.zStep() );
    for ( auto* bll : blocklets_ )
    {
	zrg.include( bll->zStart(), false );
	zrg.include( bll->zStop(), false );
    }
    return zrg;
}


void fillTrace( SeisTrc& trc, Array1D<float>& contrib )
{
    for ( auto* bll : blocklets_ )
    {
	const StepInterval<pos_type> lnrs( startlnr_, lastLineNr(*bll),
					   linerg_.step );
	bll->addData( lnrs, trc, contrib );
    }
}

};

}
}


Seis::Blocks::DataGlueer::DataGlueer( Storer& strr )
    : storer_(strr)
    , zstep_(SI().zStep())
{
}


Seis::Blocks::DataGlueer::~DataGlueer()
{
    if ( !linebufs_.isEmpty() )
    {
	pErrMsg("Call finish() before destruction and check uiRetVal");
	auto uirv = finish();
	if ( !uirv.isOK() )
	    ErrMsg( uirv );
    }
    delete arrinfo_;
}


bool Seis::Blocks::DataGlueer::is2D() const
{
    return arrinfo_ ? arrinfo_->nrDims() == 2 : false;
}


void Seis::Blocks::DataGlueer::initGeometry( const ArrayNDInfo& inf )
{
    delete arrinfo_;
    arrinfo_ = inf.clone();
}


uiRetVal Seis::Blocks::DataGlueer::addData( const Bin2D& b2d, z_type z,
					    const Arr2D& arr )
{
    if ( !arrinfo_ )
	initGeometry( arr.info() );
    if ( arrinfo_->nrDims() != 2 )
	return mINTERNAL( "Requiring 2D tiles" );
    if ( !arrinfo_->isEqual(arr.info()) )
	return mINTERNAL( "Incompatible tile passed" );

    addPos( b2d, arr, z );
    curb2d_ = b2d;
    return storeReadyPositions();
}


uiRetVal Seis::Blocks::DataGlueer::addData( const BinID& bid, z_type z,
					    const Arr3D& arr )
{
    if ( !arrinfo_ )
	initGeometry( arr.info() );
    if ( arrinfo_->nrDims() != 3 )
	return mINTERNAL( "Requiring 3D cubelets" );
    if ( !arrinfo_->isEqual(arr.info()) )
	return mINTERNAL( "Incompatible cubelet passed" );

    addPos( bid, arr, z );
    curbid_ = bid;
    return storeReadyPositions();
}


void Seis::Blocks::DataGlueer::addPos( const Bin2D& b2d,
					const Arr2D& arr, z_type z )
{
    auto* lb = getBuf( b2d.lineNr() );
    if ( !lb )
    {
	lb = new LineBuf( b2d.lineNr() );
	linebufs_ += lb;
    }
    lb->add( new Blocklet(arr,b2d.trcNr(),trcstep_,z,zstep_) );
}


void Seis::Blocks::DataGlueer::addPos( const BinID& bid,
					const Arr3D& arr, z_type z )
{
    auto* lb = getBuf( bid.inl() );
    if ( !lb )
    {
	lb = new LineBuf( bid.inl(), linestep_ );
	linebufs_ += lb;
    }
    lb->add( new Blocklet(arr,bid.crl(),trcstep_,z,zstep_) );
}


Seis::Blocks::LineBuf* Seis::Blocks::DataGlueer::getBuf( pos_type lnr )
{
    for ( auto* lb : linebufs_ )
	if ( lb->startlnr_ == lnr )
	    return lb;
    return nullptr;
}


uiRetVal Seis::Blocks::DataGlueer::finish()
{
    curbid_ = BinID::udf();
    mSetUdf( curb2d_.trcNr() );
    return storeReadyPositions();
}


uiRetVal Seis::Blocks::DataGlueer::storeReadyPositions()
{
    uiRetVal uirv;

    int nrstored = 0;
    for ( auto ilb=0; ilb<linebufs_.size(); ilb++ )
    {
	auto* lb = linebufs_.get( ilb );
	const auto lnr = is2D() ? curb2d_.lineNr() : curbid_.inl();
	if ( lb->linerg_.includes(lnr,false) )
	    break;

	uirv = storeLineBuf( *lb );
	if ( !uirv.isOK() )
	    return uirv;

	nrstored++;
    }

    for ( auto ilb=0; ilb<nrstored; ilb++ )
	delete linebufs_.removeSingle( 0 );

    return uirv;
}


uiRetVal Seis::Blocks::DataGlueer::storeLineBuf( const LineBuf& lb )
{
    const auto tnrrg = lb.trcNrRange();
    const auto zrg = lb.zRange();
    if ( trcsz_ < 0 )
	trcsz_ = zrg.nrSteps() + 1;

    SeisTrc trc( trcsz_ );
    trc.info().setGeomSystem( is2D() ? OD::LineBasedGeom : OD::VolBasedGeom );
    uiRetVal uirv;
    for ( auto lnr=lb.linerg_.start; lnr<=lb.linerg_.stop;
		lnr+=lb.linerg_.step )
    {
	trc.info().setLineNr( lnr );
	for ( auto trcnr=tnrrg.start; trcnr<=tnrrg.stop; trcnr+=trcstep_ )
	{
	    trc.info().setTrcNr( trcnr );
	    trc.info().calcCoord();
	    fillTrace( trc );
	    uirv = storer_.put( trc );
	    if ( !uirv.isOK() )
		return uirv;
	}
    }

    return uirv;
}


void Seis::Blocks::DataGlueer::fillTrace( SeisTrc& trc )
{
    Array1DImpl<float> contrib( trc.size() );
    contrib.setAll( 0.f );

    for ( auto lb : linebufs_ )
	lb->fillTrace( trc, contrib );
}
