/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisblocksdataglueer.h"

#include "arrayndalgo.h"
#include "arrayndimpl.h"
#include "point3d.h"
#include "posinfo.h"
#include "seiswrite.h"
#include "seistrc.h"
#include "sortedlist.h"
#include "survinfo.h"
#include "uistrings.h"


namespace Seis
{
typedef std::pair<float,float> floatpair;
class TraceValues
{
public:
    TraceValues(int nz)
    {
	for ( int idx=0; idx<nz; idx++ )
	    data_ += new SortedList<floatpair>;
    }

    ~TraceValues()	{}

    int size() const	{return data_.size(); }
    int size( int idx ) const
    {
	if ( !data_.validIdx(idx) )
	    return 0;
	return data_[idx]->size();
    }

    void addValue( int idx, float val, float weight )
    {
	if ( !data_.validIdx(idx) )
	    return;

	data_[idx]->add( floatpair(weight, val) );
    }

    const SortedList<floatpair>* getValues( int idx )
    {
	if ( !data_.validIdx(idx) )
	    return nullptr;
	return data_[idx];
    }

    void setEmpty()
    {
	for ( auto* lst : data_ )
	    lst->setEmpty();
    }

protected:
    ManagedObjectSet<SortedList<floatpair>>	data_;

};


class Blocklet
{
public:
Blocklet( const Array2D<float>& inp, int tnr, int tstep,
				float z, float zstep )
    : arr_(new Array2DImpl<float>(inp))
    , tnrsd_(tnr,tstep), zsd_(z,zstep)
{}

Blocklet( const Array3D<float>& inp, int tnr, int tstep,
				float z, float zstep )
    : arr_(new Array3DImpl<float>(inp))
    , tnrsd_(tnr,tstep), zsd_(z,zstep)
{}

~Blocklet()
{ delete arr_; }

    ArrayND<float>*	arr_;
    SamplingData<int>	tnrsd_;
    SamplingData<float> zsd_;

    bool	is3D() const		{ return arr_->nrDims() > 2; }
    int		nrTrcs() const	{ return arr_->getSize(arr_->nrDims()-2); }
    int		trcNrStart() const	{ return tnrsd_.start_; }
    int		trcNrStep() const	{ return tnrsd_.step_; }
    int		trcNrStop() const	{ return tnrsd_.atIndex( nrTrcs()-1 ); }
    int		nrZ() const	{ return arr_->getSize(arr_->nrDims()-1); }
    float	zStart() const		{ return zsd_.start_; }
    float	zStep() const		{ return zsd_.step_; }
    float	zStop() const		{ return zsd_.atIndex( nrZ() - 1 ); }

void addData( const StepInterval<int>& lnrs, SeisTrc& trc,
	      TraceValues& contribs, const ArrayND<float>* weights )
{
    const int lnr = trc.info().lineNr();
    const int tnr = trc.info().trcNr();
    if ( lnr < lnrs.start_ || lnr > lnrs.stop_
         || tnr < tnrsd_.start_ || tnr > trcNrStop() )
	return;

    const int nrz = nrZ();
    const StepInterval<float> trczrg = trc.zRange();
    if ( is3D() )
    {
	const int iinl = lnrs.nearestIndex( lnr );
	const int icrl = tnrsd_.nearestIndex( tnr );
	mDynamicCastGet( Array3D<float>*, arr3d, arr_ )
	mDynamicCastGet( const Array3D<float>*, w3d, weights )

	for ( int iz=0; iz<nrz; iz++ )
	{
	    const float curwt = zsd_.atIndex(iz);
	    if ( !trczrg.includes(curwt,false) )
		continue;

	    float curval = arr3d->get( iinl, icrl, iz );
	    if ( mIsUdf(curval) )
		continue;

	    const int trcidx = trczrg.nearestIndex( curwt );
	    contribs.addValue( trcidx, curval, w3d->get(iinl, icrl, iz) );
	}
    }
    else
    {
	const int itnr = tnrsd_.nearestIndex( tnr );
	mDynamicCastGet( Array2D<float>*, arr2d, arr_ )
	mDynamicCastGet( const Array2D<float>*, w2d, weights )
	for ( int iz=0; iz<nrz; iz++ )
	{
	    const float curwt = zsd_.atIndex(iz);
	    if ( !trczrg.includes(curwt,false) )
		continue;

	    float curval = arr2d->get( itnr, iz );
	    if ( mIsUdf(curval) )
		continue;

	    const int trcidx = trczrg.nearestIndex( curwt );
	    contribs.addValue( trcidx, curval, w2d->get(itnr, iz) );
	}
    }
}

};


class LineBuf
{
public:
			LineBuf( int startlnr, int lstep=1 )
			    : startlnr_(startlnr)
			    , linerg_(startlnr,startlnr,lstep)	{}
			~LineBuf()		{ deepErase(blocklets_); }

    const int		startlnr_;
    ObjectSet<Blocklet> blocklets_;
    StepInterval<int>	linerg_;


int lineNr4Idx( int lidx ) const
{
    return startlnr_ + linerg_.step_ * lidx;
}


int lastLineNr( const Blocklet& bll ) const
{
    return bll.is3D() ? lineNr4Idx( bll.arr_->getSize(0)-1 ) : startlnr_;
}


void add( Blocklet* bl )
{
    blocklets_ += bl;
    linerg_.include( lastLineNr(*bl) );
}


StepInterval<int> trcNrRange() const
{
    const Seis::Blocklet& bll0 = *blocklets_.first();
    StepInterval<int> rg( bll0.trcNrStart(), bll0.trcNrStop(),
			       bll0.trcNrStep() );
    for ( Seis::Blocklet* bll : blocklets_ )
    {
	rg.include( bll->trcNrStart(), false );
	rg.include( bll->trcNrStop(), false );
    }
    return rg;
}


StepInterval<float> zRange() const
{
    const Seis::Blocklet& bll0 = *blocklets_.first();
    StepInterval<float> zrg( bll0.zStart(), bll0.zStop(), bll0.zStep() );
    for ( Seis::Blocklet* bll : blocklets_ )
    {
	zrg.include( bll->zStart(), false );
	zrg.include( bll->zStop(), false );
    }
    return zrg;
}


void fillTrace( SeisTrc& trc, TraceValues& contribs,
		const ArrayND<float>* weights )
{
    for ( auto* bll : blocklets_ )
    {
	const StepInterval<int> lnrs( startlnr_, lastLineNr(*bll),
                                      linerg_.step_ );
	bll->addData( lnrs, trc, contribs, weights );
    }
}

};

} // namespace Seis

mDefineEnumUtils( Seis::DataGlueer, MergeMode, "Merge mode" )
{ "Average", "Crop", "Blend", nullptr };

template<>
void EnumDefImpl<Seis::DataGlueer::MergeMode>::init()
{
    uistrings_ += uiStrings::sAverage();
    uistrings_ += mEnumTr("Crop");
    uistrings_ += mEnumTr("Blend");
}


Seis::DataGlueer::DataGlueer( const TrcKeyZSampling& tkzs, SeisTrcWriter& strr,
			      Geom::Point3D<float> overlap,
			      Seis::DataGlueer::MergeMode merge )
    : tkzs_(tkzs)
    , storer_(strr)
    , mergemode_(merge)
    , overlap_(overlap)
{
}


Seis::DataGlueer::~DataGlueer()
{
    if ( !linebufs_.isEmpty() )
    {
	pErrMsg("Call finish() before destruction and check uiRetVal");
	uiRetVal uirv = finish();
	if ( !uirv.isOK() )
	    ErrMsg( uirv );
    }
    delete arrinfo_;
    delete weights_;
    delete lcd_;
}


bool Seis::DataGlueer::is2D() const
{
    return arrinfo_ ? arrinfo_->nrDims() == 2 : false;
}


void Seis::DataGlueer::initGeometry( const ArrayNDInfo& inf )
{
    delete arrinfo_;
    arrinfo_ = inf.clone();
}


uiRetVal Seis::DataGlueer::addData( const TrcKey& trcky, float z,
				    const Array2D<float>& arr )
{
    if ( !arrinfo_ )
	initGeometry( arr.info() );
    if ( arrinfo_->nrDims() != 2 )
	return mINTERNAL( "Requiring 2D tiles" );
    if ( *arrinfo_ != arr.info() )
	return mINTERNAL( "Incompatible tile passed" );
    if ( !weights_ )
    {
	weights_ = new Array2DImpl<float>( arr.info() );
	weights_->setAll( 1.f );
	TypeSet<float> winparams;
	winparams += 1.f - 2*mMaxLimited(overlap_.y_, 0.49);
	winparams += 1.f - 2*mMaxLimited(overlap_.z_, 0.49);
	PtrMan<ArrayNDWindow> wind = new ArrayNDWindow( arr.info(), true,
							"CosTaper", winparams );
	wind->apply( weights_ );
    }

    addPos( trcky, arr, z );
    curtrcky_ = trcky;
    return storeReadyPositions();
}


uiRetVal Seis::DataGlueer::addData( const TrcKey& trcky, float z,
				    const Array3D<float>& arr )
{
    if ( !arrinfo_ )
	initGeometry( arr.info() );
    if ( arrinfo_->nrDims() != 3 )
	return mINTERNAL( "Requiring 3D cubelets" );
    if ( *arrinfo_ != arr.info() )
	return mINTERNAL( "Incompatible cubelet passed" );
    if ( !weights_ )
    {
	weights_ = new Array3DImpl<float>( arr.info() );
	weights_->setAll( 1.f );
	TypeSet<float> winparams;
	winparams += 1.f - 2*mMaxLimited(overlap_.x_, 0.49);
	winparams += 1.f - 2*mMaxLimited(overlap_.y_, 0.49);
	winparams += 1.f - 2*mMaxLimited(overlap_.z_, 0.49);
	PtrMan<ArrayNDWindow> wind = new ArrayNDWindow( arr.info(), true,
							"CosTaper", winparams );
	wind->apply( weights_ );
    }

    addPos( trcky, arr, z );
    curtrcky_ = trcky;
    return storeReadyPositions();
}


void Seis::DataGlueer::addPos( const TrcKey& trcky, const Array2D<float>& arr,
				float z )
{
    Seis::LineBuf* lb = getBuf( trcky.lineNr() );
    if ( !lb )
    {
	lb = new LineBuf( trcky.lineNr() );
	linebufs_ += lb;
    }

    lb->add( new Blocklet(arr,trcky.trcNr(),trcstep_,z,tkzs_.zsamp_.step_) );
}


void Seis::DataGlueer::addPos( const TrcKey& trcky, const Array3D<float>& arr,
				float z )
{
    Seis::LineBuf* lb = getBuf( trcky.inl() );
    if ( !lb )
    {
	lb = new LineBuf( trcky.inl(), linestep_ );
	linebufs_ += lb;
    }
    lb->add( new Blocklet(arr, trcky.crl(), trcstep_, z, tkzs_.zsamp_.step_) );
}


Seis::LineBuf* Seis::DataGlueer::getBuf( int lnr )
{
    for ( Seis::LineBuf* lb : linebufs_ )
	if ( lb->startlnr_ == lnr )
	    return lb;
    return nullptr;
}


uiRetVal Seis::DataGlueer::finish()
{
    curtrcky_ = TrcKey::udf();
    uiRetVal uirv = storeReadyPositions( true );
    if ( !storer_.close() )
	uirv.add( storer_.errMsg() );

    return uirv;
}


uiRetVal Seis::DataGlueer::storeReadyPositions( bool force )
{
    uiRetVal uirv;

    int nrstored = 0;
    for ( int ilb=0; ilb<linebufs_.size(); ilb++ )
    {
	Seis::LineBuf* lb = linebufs_.get( ilb );
	const TrcKey& trcky = curtrcky_;
	const int lnr = trcky.lineNr();
	if ( !force && lb->linerg_.includes(lnr,false) )
	    break;

	uirv = storeLineBuf( *lb );
	if ( !uirv.isOK() )
	    return uirv;

        lastwrittenline_ = lb->linerg_.stop_;
	nrstored++;
    }

    for ( int ilb=0; ilb<nrstored; ilb++ )
	delete linebufs_.removeSingle( 0 );

    return uirv;
}


StepInterval<int> Seis::DataGlueer::trcNrRange( int lnr ) const
{
    bool initialized = false;
    StepInterval<int> rg = StepInterval<int>::udf();
    for ( Seis::LineBuf* lb : linebufs_ )
    {
	if ( !lb->linerg_.includes(lnr,false) )
	    continue;

	if ( initialized )
	    rg.include( lb->trcNrRange(), false );
	else
	{
	    rg = lb->trcNrRange();
	    initialized = true;
	}
    }

    return rg;
}


uiRetVal Seis::DataGlueer::storeLineBuf( const LineBuf& lb )
{
    const StepInterval<float> zrg = tkzs_.zsamp_;
    const int nrz = tkzs_.nrZ();

    SeisTrc trc( nrz );
    TraceValues contribs( nrz );
    trc.info().sampling_.start_ = zrg.start_;
    trc.info().sampling_.step_ = zrg.step_;
    trc.info().setGeomSystem( is2D() ? OD::Geom2D : OD::Geom3D );
    uiRetVal uirv;
    for ( int lnr=lb.linerg_.start_; lnr<=lb.linerg_.stop_;
	  lnr+=lb.linerg_.step_ )
    {
	if ( lnr <= lastwrittenline_ )
	    continue;

	if ( is2D() )
	    trc.info().setGeomID( Pos::GeomID(lnr) );
	else
	    trc.info().setLineNr( lnr );

	const StepInterval<int> tnrrg = trcNrRange( lnr );
        for ( int trcnr=tnrrg.start_; trcnr<=tnrrg.stop_; trcnr+=trcstep_ )
	{
	    trc.info().setTrcNr( trcnr );
	    if ( !tkzs_.hsamp_.includes(trc.info().trcKey()) ||
		(lcd_ && !lcd_->includes(trc.info().binID())) )
		continue;

	    trc.info().coord_ = Survey::GM().toCoord( trc.info().trcKey() );
	    fillTrace( trc, contribs );
	    if ( !storer_.put(trc) )
		uirv = storer_.errMsg();

	    if ( !uirv.isOK() )
		return uirv;
	}
    }

    return uirv;
}


void Seis::DataGlueer::fillTrace( SeisTrc& trc, TraceValues& contribs )
{
    trc.zero();
    contribs.setEmpty();
    for ( Seis::LineBuf* lb : linebufs_ )
	lb->fillTrace( trc, contribs, weights_ );

    for ( int idz=0; idz<trc.size(); idz++ )
    {
	const SortedList<floatpair>& contrib = *contribs.getValues( idz );
	float trcval = 0.f;
	if ( contrib.isEmpty() )
	    trcval = mUdf(float);
	else
	{
	    const int sz = contrib.size();
	    if ( sz==1 )
		trcval = contrib[0].second;
	    else if ( mergemode_==Average )
	    {
		for ( const auto& vals : contrib.vec() )
		    trcval += vals.second;
		trcval /= float( sz );
	    }
	    else if ( mergemode_==Crop )
		trcval = contrib[sz-1].second;
	    else	// Blend
	    {
		float weight = 0.0;
		for ( const auto& vals : contrib.vec() )
		{
		    trcval += vals.first * vals.second;
		    weight += vals.first;
		}
		trcval /= weight;
	    }
	}
	trc.set( idz, trcval, 0 );
    }
}


void Seis::DataGlueer::setTracePositions( const PosInfo::CubeData* lcd)
{
    deleteAndNullPtr( lcd_ );
    if ( lcd )
	lcd_ = new PosInfo::CubeData( *lcd );
}
