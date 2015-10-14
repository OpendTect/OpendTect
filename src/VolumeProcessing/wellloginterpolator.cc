/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellloginterpolator.h"

#include "arrayndimpl.h"
#include "gridder2d.h"
#include "interpollayermodel.h"
#include "seisdatapack.h"
#include "survinfo.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellreader.h"
#include "welltrack.h"


namespace VolProc
{

static const char* sKeyNrWells()	{ return "Nr of wells"; }
static const char* sKeyWellLogID()	{ return "WellLog ID"; }
static const char* sKeyLogName()	{ return "Log name"; }
static const char* sKeyAlgoName()	{ return "Algorithm"; }
static const char* sKeyExtension()	{ return "Output boundary"; }
static const char* sKeyLogExtension()	{ return "Logs Extension"; }
static const char* sKeyLayerModel()	{ return "Layer Model"; }


class WellLogInfo
{
public:
WellLogInfo( const MultiID& mid, const char* lognm )
    : mid_(mid), logname_(lognm)
{}

~WellLogInfo()
{
    delete track_;
    delete log_;
}

bool init()
{
    const bool isloaded = Well::MGR().isLoaded( mid_ );
    RefMan<Well::Data> wd = new Well::Data;
    if ( isloaded && !wd )
    {
	return false;
    }
    else if ( !isloaded )
    {
	Well::Reader wrdr( mid_, *wd );
	if ( !wrdr.getTrack() ||
	     ( SI().zIsTime() && ( !wrdr.getInfo() || !wrdr.getD2T() ) ) ||
	     !wrdr.getLog(logname_) )
	    return false;
    }

    if ( SI().zIsTime() )
    {
	track_ = new Well::Track;
	track_->toTime( *wd );
    }
    else
	track_ = isloaded ? &wd->track() : new Well::Track( wd->track() );

    const Well::Log* log = wd->logs().getLog( logname_ );
    log_ = isloaded ? log : new Well::Log( *log );

    return true;
}

void computeLayerModelIntersection(
	const InterpolationLayerModel& layermodel )
{
    intersections_.setSize( layermodel.nrLayers(), mUdf(float) );

    StepInterval<float> dahrg = track_->dahRange(); dahrg.step = 5;
    const int nrdah = dahrg.nrSteps() + 1;
    for ( int lidx=0; lidx<layermodel.nrLayers(); lidx++ )
    {
	for ( int dahidx=1; dahidx<nrdah; dahidx++ )
	{
	    const float prevdah = dahrg.atIndex( dahidx-1 );
	    const float dah = dahrg.atIndex( dahidx );
	    const Coord3 prevpos = track_->getPos( prevdah );
	    const Coord3 pos = track_->getPos( dah );
	    const float prevlayerz =
		layermodel.getZ( SI().transform(prevpos), lidx );
	    const float layerz = layermodel.getZ( SI().transform(pos), lidx );
	    const float avgz = (prevlayerz+layerz) / 2;
	    if ( avgz>=prevpos.z && avgz<pos.z )
	    {
		const float slope = (dah-prevdah) / float(pos.z-prevpos.z);
		const float calcdah = prevdah + slope*float(avgz-prevpos.z);
		intersections_[lidx] = calcdah;
		break;
	    }
	}
    }
}


Well::Track*	track_;
TrcKeyZSampling bbox_;
MultiID		mid_;
BufferString	logname_;
const Well::Log*	log_;
TypeSet<float>	intersections_;

};


WellLogInterpolator::WellLogInterpolator()
    : gridder_(0)
    , extension_(ExtrapolateEdgeValue)
    , extlog_(0)
    , layermodel_(0)
{}


WellLogInterpolator::~WellLogInterpolator()
{
    releaseData();
}


void WellLogInterpolator::releaseData()
{
    Step::releaseData();
    delete gridder_; gridder_ = 0;
    delete layermodel_; layermodel_ = 0;
}


void WellLogInterpolator::setLayerModel( InterpolationLayerModel* mdl )
{
    delete layermodel_;
    layermodel_ = mdl;
}


const InterpolationLayerModel* WellLogInterpolator::getLayerModel() const
{ return layermodel_; }


void WellLogInterpolator::setGridder( const char* nm, float radius )
{
    delete gridder_;

    if ( FixedString(nm) == RadialBasisFunctionGridder2D::sFactoryKeyword() )
    {
	gridder_ = new RadialBasisFunctionGridder2D();
	return;
    }

    if ( FixedString(nm) ==  TriangulatedGridder2D::sFactoryKeyword() )
    {
	gridder_ = new TriangulatedGridder2D();
	return;
    }

    if ( FixedString(nm) == InverseDistanceGridder2D::sFactoryKeyword() )
    {
	InverseDistanceGridder2D* invgrid = new InverseDistanceGridder2D();
	gridder_ = invgrid;
	if ( !mIsUdf(radius) )
	    invgrid->setSearchRadius( radius );

	return;
    }
}


const char* WellLogInterpolator::getGridderName() const
{
    mDynamicCastGet( InverseDistanceGridder2D*, invgrid, gridder_ );
    if ( invgrid )
	return invgrid->factoryKeyword();

    mDynamicCastGet( TriangulatedGridder2D*, trigrid, gridder_ );
    if ( trigrid )
	return trigrid->factoryKeyword();

    mDynamicCastGet( RadialBasisFunctionGridder2D*, rbfgrid, gridder_ );
    if ( rbfgrid )
	return rbfgrid->factoryKeyword();

    return 0;
}


float WellLogInterpolator::getSearchRadius() const
{
    mDynamicCastGet( InverseDistanceGridder2D*, invgrid, gridder_ );
    return invgrid ? invgrid->getSearchRadius() : mUdf( float );
}


bool WellLogInterpolator::is2D() const
{ return false; }


void WellLogInterpolator::setWellData( const TypeSet<MultiID>& ids,
				       const char* lognm )
{
    wellmids_ = ids;
    logname_ = lognm;
}


void WellLogInterpolator::getWellNames( BufferStringSet& res ) const
{
    for ( int idx=0; idx<wellmids_.size(); idx++ )
    {
	Well::Data* data = Well::MGR().get( wellmids_[idx] );
	if ( data )
	    res.add( data->name() );
    }
}


const char* WellLogInterpolator::getLogName() const
{ return logname_.buf(); }

void WellLogInterpolator::getWellIDs( TypeSet<MultiID>& ids ) const
{ ids = wellmids_; }


bool WellLogInterpolator::prepareComp( int )
{
    if ( !layermodel_ || !layermodel_->prepare(0) )
	return false;

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || output->isEmpty() || !gridder_ || is2D() )
	return false;

    const TrcKeySampling& hs = output->sampling().hsamp_;
    outputinlrg_ = hs.inlRange();

    outputcrlrg_ = hs.crlRange();

    bool res = true;
    for ( int idx=0; idx<wellmids_.size(); idx++ )
    {
	WellLogInfo* info = new WellLogInfo( wellmids_[idx], logname_ );
	info->init();
	info->computeLayerModelIntersection( *layermodel_ );
	infos_ += info;
    }

    return res;
}


static TypeSet<float> getMDs( const WellLogInfo& info, float layeridx )
{
    TypeSet<float> mds;
    const float fidx0 = Math::Floor( layeridx );
    const float fidx1 = Math::Ceil( layeridx );
    const int idx0 = mNINT32(fidx0);
    const int idx1 = mNINT32(fidx1);
    if ( !info.intersections_.validIdx(idx0) ||
	 !info.intersections_.validIdx(idx1) )
	return mds;

    const float dah0 = info.intersections_[idx0];
    const float dah1 = info.intersections_[idx1];
    if ( mIsZero(fidx1-fidx0,mDefEps) )
	mds += dah0;
    else
    {
	const float slope = (dah1-dah0) / (fidx1-fidx0);
	mds += dah0 + slope*(layeridx-fidx0);
    }

    return mds;
}


bool WellLogInterpolator::computeBinID( const BinID& bid, int )
{
    if ( !outputinlrg_.includes( bid.inl(), true ) ||
	 !outputcrlrg_.includes( bid.crl(), true ) ||
         (bid.inl()-outputinlrg_.start)%outputinlrg_.step ||
         (bid.crl()-outputcrlrg_.start)%outputcrlrg_.step )
	return true;

    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    Array3D<float>& outputarray = output->data(0);
    const int lastzidx = outputarray.info().getSize(2) - 1;

    BinID nearbid = bid;

    PtrMan<Gridder2D> gridder = gridder_->clone();
    const TrcKeySampling& hs = output->sampling().hsamp_;
    const Coord gridpoint( hs.toCoord(nearbid) );

    mAllocVarLenArr(float,vals,lastzidx+1);
    int lasthcidx=-1, firsthcidx=-1;
    for ( int idx=lastzidx; idx>=0; idx-- )
    {
	vals[idx] = mUdf(float);
	const float z = output->sampling().zsamp_.atIndex( idx );
	const float layeridx = layermodel_->getLayerIndex( bid, z );
	if ( mIsUdf(layeridx) ) continue;

	TypeSet<Coord> wellposes;
	TypeSet<float> logvals;
	for ( int idy=0; idy<infos_.size(); idy++ )
	{
	    if ( !infos_.validIdx(idy) ) continue;
	    WellLogInfo* info = infos_[idy];
	    const Well::Log* log = info ? info->log_ : 0;
	    if ( !log ) continue;

	    TypeSet<float> mds = getMDs( *info, layeridx );
	    if ( mds.isEmpty() )
		continue;

	    for ( int idz=0; idz<mds.size(); idz++ )
	    {
		const float lv = log->getValue( mds[idz], extlog_ );
		if ( mIsUdf(lv) )
		    continue;

		const Coord pos = info->track_->getPos( mds[idz] );
		if ( pos.isUdf() )
		    continue;

		wellposes += pos;
		logvals += lv;
	    }
	}

	gridder->setPoints( wellposes );
	gridder->setValues( logvals );
	vals[idx] = gridder->getValue( gridpoint );
	if ( extension_==None )
	    continue;

	if ( lasthcidx==-1 )
	{
	    lasthcidx = idx;
	    firsthcidx = idx;
	}
	else if ( idx<firsthcidx )
	    firsthcidx = idx;
    }

    const int outputinlidx = outputinlrg_.nearestIndex( bid.inl() );
    const int outputcrlidx = outputcrlrg_.nearestIndex( bid.crl() );
    const bool useextension = extension_!=None && lasthcidx!=-1;
    for ( int idx=lastzidx; idx>=0; idx-- )
    {
	float val = vals[idx];
	if ( useextension )
	{
	    if ( idx>lasthcidx )
		val = vals[lasthcidx];
	    else if ( idx<firsthcidx )
		val = vals[firsthcidx];
	}

	outputarray.set( outputinlidx, outputcrlidx, idx, val );
    }

    return true;
}


void WellLogInterpolator::fillPar( IOPar& pars ) const
{
    Step::fillPar( pars );

    pars.set( sKeyExtension(), extension_ );
    pars.setYN( sKeyLogExtension(), extlog_ );

    pars.set( sKeyAlgoName(), getGridderName() );
    if ( gridder_ )
	gridder_->fillPar( pars );

    pars.set( sKeyLogName(), logname_ );
    pars.set( sKeyNrWells(), wellmids_.size() );
    for ( int idx=0; idx<wellmids_.size(); idx++ )
    {
	const BufferString key = IOPar::compKey( sKeyWellLogID(), idx );
	pars.set( key, wellmids_[idx] );
    }

    if ( layermodel_ )
    {
	IOPar lmpar;
	layermodel_->fillPar( lmpar );
	pars.mergeComp( lmpar, sKeyLayerModel() );
    }
}


bool WellLogInterpolator::usePar( const IOPar& pars )
{
    if ( !Step::usePar(pars) )
	return false;

    int extension = 0;
    pars.get( sKeyExtension(), extension );
    extension_ = (ExtensionModel)extension;

    pars.getYN( sKeyLogExtension(), extlog_ );
    pars.get( sKeyLogName(), logname_ );

    wellmids_.erase();
    int nrwells = 0;
    pars.get( sKeyNrWells(), nrwells );
    for ( int idx=0; idx<nrwells; idx++ )
    {
	MultiID mid;
	const BufferString key = IOPar::compKey( sKeyWellLogID(), idx );
	if ( pars.get(key,mid) )
	    wellmids_ += mid;
    }

    float radius = 0;;
    pars.get( InverseDistanceGridder2D::sKeySearchRadius(), radius );
    BufferString nm;
    pars.get( sKeyAlgoName(), nm );
    setGridder( nm.buf(), radius );

    delete layermodel_; layermodel_ = 0;
    PtrMan<IOPar> lmpar = pars.subselect( sKeyLayerModel() );
    if ( lmpar )
    {
	BufferString lmtype;
	lmpar->get( InterpolationLayerModel::sKeyModelType(), lmtype );
	layermodel_ = InterpolationLayerModel::factory().create( lmtype );
	if ( !layermodel_ || !layermodel_->usePar(*lmpar) )
	{ delete layermodel_; layermodel_ = 0; }
    }

    return true;
}

} // namespace VolProc

