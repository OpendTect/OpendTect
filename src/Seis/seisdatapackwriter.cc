/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2007
________________________________________________________________________

-*/

#include "seisdatapackwriter.h"

#include "arrayndimpl.h"
#include "horsubsel.h"
#include "ioobj.h"
#include "keystrs.h"
#include "posinfo.h"
#include "scaler.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisrangeseldata.h"
#include "seisstorer.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "uistrings.h"
#include "unitofmeasure.h"


SeisDataPackWriter::SeisDataPackWriter( const DBKey& outid,
				  const RegularSeisDataPack& dp,
				  const TypeSet<int>& compidxs )
    : Executor( "Attribute volume writer" )
    , nrdone_( 0 )
    , tks_( dp.sampling().hsamp_ )
    , cubezrgidx_(0,dp.sampling().nrZ()-1)
    , totalnr_( (int) dp.sampling().hsamp_.totalNr() )
    , dp_( &dp )
    , iterator_( dp.sampling().hsamp_ )
    , outid_( outid )
    , posinfo_(0)
    , compidxs_( compidxs )
{
    compscalers_.setNullAllowed( true );
    getPosInfo();

    if ( compidxs_.isEmpty() )
    {
	for ( int idx=0; idx<dp_->nrComponents(); idx++ )
	{
	    compidxs_ += idx;
	    compscalers_ += 0;
	}
    }
    else
    {
	for ( int idx=0; idx<compidxs_.size(); idx++ )
	    compscalers_ += 0;
    }

    PtrMan<IOObj> ioobj = outid_.getIOObj();
    storer_ = ioobj ? new Seis::Storer( *ioobj ) : 0;
    is2d_ = storer_ && storer_->is2D();

    msg_ = storer_ ? uiStrings::sWriting()
		   : uiStrings::phrCannotWrite( uiStrings::sData() );
}


SeisDataPackWriter::~SeisDataPackWriter()
{
    delete trc_;
    delete seldata_;
    delete storer_;
    deepErase( compscalers_ );
}


void SeisDataPackWriter::getPosInfo()
{
    const PosInfo::CubeData* pi = dp_->trcsSampling();
    posinfo_ = pi;
    if ( pi && !pi->isFullyRectAndReg() )
	totalnr_ = posinfo_->totalSizeInside( CubeHorSubSel(tks_) );
}


void SeisDataPackWriter::setComponentScaler( const Scaler& scaler, int compidx )
{
    if ( scaler.isEmpty() )
	return;

    for ( int idx=0; idx<=compidx; idx++ )
    {
	if ( !compscalers_.validIdx(idx) )
	    compscalers_ += 0;
    }

    delete compscalers_.replace( compidx, scaler.clone() );
    adjustSteeringScaler( compidx );
}


void SeisDataPackWriter::adjustSteeringScaler( int compidx )
{
    if ( !storer_ || !dp_ || !dp_->is2D() ||
	 !compscalers_[compidx] || compscalers_[compidx]->isEmpty() )
	return;

    const SeisIOObjInfo objinfo( outid_ );
    if ( !objinfo.isOK() || !objinfo.is2D() )
	return;

    const IOObj* outioobj = objinfo.ioObj();
    BufferString type;
    if ( !outioobj || !outioobj->pars().get(sKey::Type(),type) ||
	 type != BufferString(sKey::Steering()) )
	return;

    const Pos::GeomID geomid = dp_->sampling().hsamp_.getGeomID();
    const auto& geom2d = Survey::Geometry::get2D( geomid );
    if ( geom2d.isEmpty() )
	return;

    double trcdist = geom2d.averageTrcDist();
    const UnitOfMeasure* feetuom = UnitOfMeasure::feetUnit();
    if ( feetuom && SI().xyInFeet() )
	trcdist = feetuom->getSIValue( trcdist );

    double zstep = dp_->sampling().zsamp_.step;
    const UnitOfMeasure* zuom = UnitOfMeasure::surveyDefZUnit();
    const ZDomain::Def& zdef = SI().zDomain();
    if ( zuom && zdef.isDepth() )
	zstep = zuom->getSIValue( zstep );

    const UnitOfMeasure* zdipuom = zdef.isDepth() ? UoMR().get( "Millimeters" )
						  : UoMR().get( "Microseconds");
    const UnitOfMeasure* targetzuom = zdipuom;
    if ( targetzuom )
	zstep = targetzuom->getUserValueFromSI( zstep );

    const double scalefactor = zstep / trcdist;
    const LinScaler dipscaler( 0., scalefactor );
    delete compscalers_.replace( compidx, dipscaler.clone() );
}


od_int64 SeisDataPackWriter::nrDone() const
{
    return nrdone_;
}


void SeisDataPackWriter::setNextDataPack( const RegularSeisDataPack& dp )
{
    if ( dp_ != &dp )
    {
        dp_ = &dp;
	getPosInfo();
    }

    nrdone_ = 0;
    if ( cubezrgidx_.stop >= dp.sampling().nrZ() )
	cubezrgidx_.stop = dp.sampling().nrZ()-1;

    setSelection( dp_->sampling().hsamp_ );
}


void SeisDataPackWriter::setSelection( const TrcKeySampling& hrg,
				       const Interval<int>* cubezrgidx )
{
    tks_ = hrg;
    iterator_.setSampling( hrg );
    totalnr_ = posinfo_ ? posinfo_->totalSizeInside( CubeHorSubSel(hrg) )
			: mCast(int,hrg.totalNr());
    delete seldata_;
    seldata_ = new Seis::RangeSelData( tks_ );

    if ( !cubezrgidx )
	return;

    if ( cubezrgidx->stop >= dp_->sampling().nrZ() )
	{ msg_ = mINTERNAL("Invalid selection"); cubezrgidx_.setUdf(); return; }

    cubezrgidx_ = *cubezrgidx;
}


od_int64 SeisDataPackWriter::totalNr() const
{
    return totalnr_;
}


bool SeisDataPackWriter::goImpl( od_ostream* strm, bool first, bool last,
				 int delay )
{
    const bool success = Executor::goImpl( strm, first, last, delay );
    dp_ = 0; posinfo_ = 0;
    deleteAndZeroPtr( trc_ );

    return success;
}


bool SeisDataPackWriter::setTrc()
{
    if ( !storer_ || dp_->isEmpty() )
	return false;

    if ( cubezrgidx_.isUdf() )
	{ msg_ = mINTERNAL("No cubezrgidx_"); return false; }

    const int trcsz = cubezrgidx_.stop - cubezrgidx_.start + 1;
    delete trc_;
    trc_ = new SeisTrc( trcsz );

    trc_->info().sampling_.start = dp_->sampling().zsamp_.atIndex(
				  cubezrgidx_.start );
    trc_->info().sampling_.step = dp_->sampling().zsamp_.step;

    BufferStringSet compnames;
    compnames.add( dp_->getComponentName() );
    for ( int idx=1; idx<compidxs_.size(); idx++ )
    {
	trc_->data().addComponent( trcsz, DataCharacteristics() );
	compnames.add( dp_->getComponentName(idx) );
    }

    SeisTrcTranslator* transl = storer_->translator();
    if ( transl )
	transl->setComponentNames( compnames );

    return true;
}


int SeisDataPackWriter::nextStep()
{
    if ( !trc_ && !setTrc() )
	return ErrorOccurred();

    const TrcKeySampling& hs = dp_->sampling().hsamp_;
    const od_int64 posidx = iterator_.curIdx();
    if ( posinfo_ )
    {
	const BinID bid( hs.atIndex(posidx) );
	if ( !posinfo_->isValid(posinfo_->cubeDataPos(bid)) )
	    return iterator_.next() ? MoreToDo() : Finished();
    }
    const TrcKey currentpos( iterator_.curTrcKey() );
    if ( seldata_ && !seldata_->isOK(currentpos) )
	return MoreToDo();

    trc_->info().trcKey() = currentpos;
    trc_->info().coord_ = currentpos.getCoord();
    const int inlpos = hs.lineIdx( currentpos.inl() );
    const int crlpos = hs.trcIdx( currentpos.crl() );
    const int trcsz = cubezrgidx_.stop - cubezrgidx_.start + 1;
    int cubesample = cubezrgidx_.start;
    const od_int64 offset = dp_->data().info().
					getOffset( inlpos, crlpos, cubesample );
    float value = mUdf(float);
    for ( int compidx=0; compidx<compidxs_.size(); compidx++ )
    {
	const Array3D<float>& outarr = dp_->data( compidxs_[compidx] );
	const float* dataptr = outarr.getData();
	const Scaler* scaler = compscalers_[compidx];
	if ( dataptr ) dataptr += offset;
	cubesample = cubezrgidx_.start;
	for ( int idz=0; idz<trcsz; idz++ )
	{
	    value = dataptr ? *dataptr++
			    : outarr.get( inlpos, crlpos, cubesample++ );

	    if ( scaler )
		value = mCast(float,scaler->scale( value ));

	    trc_->set( idz, value, compidx );
	}
    }

    auto uirv = storer_->put( *trc_ );
    if ( !uirv.isOK() )
	{ msg_ = uirv; return ErrorOccurred(); }

    nrdone_++;
    if ( iterator_.next() )
	return MoreToDo();

    uirv = storer_->close();
    if ( !uirv.isOK() )
	{ msg_ = uirv; return ErrorOccurred(); }

    return Finished();
}
