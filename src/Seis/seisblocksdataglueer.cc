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
mUseType( DataGlueer, Arr2D );
mUseType( DataGlueer, Arr3D );
typedef ArrayND<z_type> ArrND;

class Blocklet
{
public:

    Blocklet( const Arr2D& inp, z_type z )
	: data_(new Array2DImpl<val_type>(inp)), midz_(z)	    {}
    Blocklet( const Arr3D& inp, z_type z )
	: data_(new Array3DImpl<val_type>(inp)), midz_(z)	    {}
    ~Blocklet()	{ delete data_; }

    ArrND*	data_;
    z_type	midz_;

};


class LineBuf
{
public:

    mUseType( DataGlueer,	Arr2D );
    mUseType( DataGlueer,	Arr3D );
    mUseType( DataGlueer,	idx_type );
    mUseType( DataGlueer,	pos_type );

			LineBuf( pos_type lnr )
			    : lnr_(lnr)		{}
			~LineBuf()		{ deepErase(blocklets_); }

idx_type idxOf( pos_type tnr ) const
{
    for ( auto idx=0; idx<trcnrs_.size(); idx++ )
	if ( trcnrs_[idx] == tnr )
	    return idx;
    return -1;
}

void add( pos_type tnr, Blocklet* bl )
{
    blocklets_ += bl;
    trcnrs_ += tnr;
}

Interval<pos_type> trcNrRange( pos_type tnrwdth ) const
{
    return Interval<pos_type>( trcnrs_.first()-tnrwdth, trcnrs_.last()+tnrwdth);
}


StepInterval<z_type> zRange( int zso, z_type zstep ) const
{
    StepInterval<z_type> zrg;
    zrg.start = zrg.stop = blocklets_.first()->midz_;
    for ( auto* bll : blocklets_ )
	zrg.include( bll->midz_, false );

    zrg.start -= zstep * zso;
    zrg.stop += zstep * zso;
    zrg.step = zstep;
    return zrg;
}

    const pos_type	lnr_;
    TypeSet<pos_type>	trcnrs_;
    ObjectSet<Blocklet>	blocklets_;

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


uiRetVal Seis::Blocks::DataGlueer::addData( const Bin2D& b2d, z_type midz,
					    const Arr2D& arr )
{
    if ( !arrinfo_ )
	initGeometry( arr.info() );
    if ( arrinfo_->nrDims() != 2 )
	return mINTERNAL( "Requiring 2D tiles" );
    if ( !arrinfo_->isEqual(arr.info()) )
	return mINTERNAL( "Incompatible tile passed" );

    addPos( b2d, arr, midz );
    curb2d_ = b2d;
    return storeReadyPositions();
}


uiRetVal Seis::Blocks::DataGlueer::addData( const BinID& bid, z_type midz,
					    const Arr3D& arr )
{
    if ( !arrinfo_ )
	initGeometry( arr.info() );
    if ( arrinfo_->nrDims() != 3 )
	return mINTERNAL( "Requiring 3D cubelets" );
    if ( !arrinfo_->isEqual(arr.info()) )
	return mINTERNAL( "Incompatible cubelet passed" );

    addPos( bid, arr, midz );
    curbid_ = bid;
    return storeReadyPositions();
}


void Seis::Blocks::DataGlueer::addPos( const Bin2D& b2d,
					const Arr2D& arr, z_type midz )
{
    auto* lb = getBuf( b2d.lineNr() );
    if ( !lb )
	lb = new LineBuf( b2d.lineNr() );
    lb->add( b2d.trcNr(), new Blocklet(arr,midz) );
}


void Seis::Blocks::DataGlueer::addPos( const BinID& bid,
					const Arr3D& arr, z_type midz )
{
    auto* lb = getBuf( bid.inl() );
    if ( !lb )
	lb = new LineBuf( bid.inl() );
    lb->add( bid.crl(), new Blocklet(arr,midz) );
}


Seis::Blocks::LineBuf* Seis::Blocks::DataGlueer::getBuf( pos_type lnr )
{
    for ( auto* lb : linebufs_ )
	if ( lb->lnr_ == lnr )
	    return lb;
    return nullptr;
}


uiRetVal Seis::Blocks::DataGlueer::finish()
{
    curbid_ = BinID::udf();
    mSetUdf( curb2d_.trcNr() );
    return storeReadyPositions();
}


int Seis::Blocks::DataGlueer::stepoutSize( int idim ) const
{
    if ( !arrinfo_ )
	{ pErrMsg("stepoutSize unknown before first add"); return 0; }
    return arrinfo_->getSize( idim ) / 2;
}


Seis::Blocks::DataGlueer::pos_type Seis::Blocks::DataGlueer::trcNrWidth() const
{
    return stepoutSize( is2D() ? 0 : 1 ) * trcstep_;
}


Seis::Blocks::DataGlueer::pos_type Seis::Blocks::DataGlueer::inlineWidth() const
{
    return stepoutSize(0) * linestep_;
}


uiRetVal Seis::Blocks::DataGlueer::storeReadyPositions()
{
    uiRetVal uirv;

    for ( auto* lb : linebufs_ )
    {
	const bool isfinishedline = is2D() ? lb->lnr_ != curb2d_.lineNr()
				: lb->lnr_ < curbid_.inl() - 2*inlineWidth();
	if ( !isfinishedline )
	    break;

	uirv = storeLine( *lb );
	if ( !uirv.isOK() )
	    return uirv;
    }

    return uirv;
}


uiRetVal Seis::Blocks::DataGlueer::storeLine( const LineBuf& lb )
{
    const auto firstln = is2D() ? lb.lnr_ : lb.lnr_ - inlineWidth();
    const auto lastln = is2D() ? lb.lnr_ : lb.lnr_ + inlineWidth();

    const auto tnrrg = lb.trcNrRange( trcNrWidth() );
    const auto zrg = lb.zRange( stepoutSize(is2D()?1:2), zstep_ );
    if ( trcsz_ < 0 )
	trcsz_ = zrg.nrSteps() + 1;

    SeisTrc trc( trcsz_ );
    trc.info().setGeomSystem( is2D() ? OD::LineBasedGeom : OD::VolBasedGeom );
    uiRetVal uirv;
    for ( auto lnr=firstln; lnr<=lastln; lnr++ )
    {
	trc.info().setLineNr( lnr );
	for ( auto trcnr=tnrrg.start; trcnr<=tnrrg.stop; trcnr+=trcstep_ )
	{
	    trc.info().setTrcNr( trcnr );
	    trc.info().calcCoord();
	    //TODO use multiplicity trace and collect all contributions
	    uirv = storer_.put( trc );
	    if ( !uirv.isOK() )
		return uirv;
	}
    }

    return uirv;
}
