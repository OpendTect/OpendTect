/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellloginterpolator.h"

#include "arraynd.h"
#include "gridder2d.h"
#include "interpollayermodel.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
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
    : mid_(mid), logname_(lognm), wd_(*new Well::Data)
{}

~WellLogInfo()
{ delete &wd_; }

bool init()
{
    PtrMan<IOObj> ioobj = IOM().get( mid_ );
    if ( !ioobj ) return false;

    Well::Reader rdr( ioobj->fullUserExpr(true), wd_ );
    if ( !rdr.getTrack() || !rdr.getD2T() || !rdr.getLog(logname_) )
	return false;

    PtrMan<Well::Track> timetrack = 0;
    if ( SI().zIsTime() )
    {
	timetrack = new Well::Track( wd_.track() );
	timetrack->toTime( *wd_.d2TModel(), wd_.track() );
    }

    const Well::Track& track = SI().zIsTime() ? *timetrack : wd_.track();
    trackpos_ = track.getAllPos();
    return true;
}

void computeLayerModelIntersection(
	const InterpolationLayerModel& layermodel )
{
    intersections_.setSize( layermodel.nrLayers(), mUdf(float) );

    PtrMan<Well::Track> timetrack = 0;
    if ( SI().zIsTime() )
    {
	timetrack = new Well::Track( wd_.track() );
	timetrack->toTime( *wd_.d2TModel(), wd_.track() );
    }

    const Well::Track& track = SI().zIsTime() ? *timetrack : wd_.track();
    StepInterval<float> dahrg = track.dahRange(); dahrg.step = 5;
    const int nrdah = dahrg.nrSteps() + 1;
    for ( int lidx=0; lidx<layermodel.nrLayers(); lidx++ )
    {
	for ( int dahidx=1; dahidx<nrdah; dahidx++ )
	{
	    const float prevdah = dahrg.atIndex( dahidx-1 );
	    const float dah = dahrg.atIndex( dahidx );
	    const Coord3 prevpos = track.getPos( prevdah );
	    const Coord3 pos = track.getPos( dah );
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


Well::Data&	wd_;
CubeSampling	bbox_;
MultiID		mid_;
BufferString	logname_;
TypeSet<Coord3>	trackpos_;
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


bool WellLogInterpolator::setLayerModel( const IOPar& par )
{
    if ( layermodel_ )
    {
	delete layermodel_;
	layermodel_ = 0;
    }

    BufferString nm;
    par.get( InterpolationLayerModel::sKeyModelType(), nm );
    layermodel_ = InterpolationLayerModel::factory().create( nm );
    return layermodel_ ? layermodel_->usePar( par ) : false;
}


bool WellLogInterpolator::getLayerModel( IOPar& par ) const
{
    if ( !layermodel_ ) return false;

    layermodel_->fillPar( par );
    return true;
}


void WellLogInterpolator::setGridder( const char* nm, float radius )
{
    if ( gridder_ )
	delete gridder_;

    if ( FixedString(nm) != InverseDistanceGridder2D::sFactoryKeyword() )
	gridder_ = new TriangulatedGridder2D();
    else
    {
	InverseDistanceGridder2D* invgrid = new InverseDistanceGridder2D();
	if ( !mIsUdf(radius) )
	    invgrid->setSearchRadius(radius);
	gridder_ = invgrid;
    }
}


const char* WellLogInterpolator::getGridderName() const
{
    mDynamicCastGet( InverseDistanceGridder2D*, invgrid, gridder_ );
    if ( invgrid )
	return invgrid->factoryKeyword();
    else
    {
	mDynamicCastGet( TriangulatedGridder2D*, trigrid, gridder_ );
	if ( trigrid )
	    return trigrid->factoryKeyword();
    }

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
    Attrib::DataCubes* output = getOutput( getOutputSlotID(0) );
    if ( !output || !output->nrCubes() || !gridder_ || is2D() )
	return false;

    outputinlrg_ = StepInterval<int>( output->inlsampling_.start,
	output->inlsampling_.atIndex( output->getInlSz()-1 ),
	output->inlsampling_.step );

    outputcrlrg_ = StepInterval<int>( output->crlsampling_.start,
	output->crlsampling_.atIndex( output->getCrlSz()-1 ),
	output->crlsampling_.step );

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
    const float idx0 = floor( layeridx );
    const float idx1 = ceil( layeridx );
    const float dah0 = info.intersections_[ mNINT32(idx0) ];
    const float dah1 = info.intersections_[ mNINT32(idx1) ];
    if ( mIsZero(idx1-idx0,mDefEps) )
	mds += dah0;
    else
    {
	const float slope = (dah1-dah0) / (idx1-idx0);
	mds += dah0 + slope*(layeridx-idx0);
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

    Attrib::DataCubes* output = getOutput( getOutputSlotID(0) );
    Array3D<float>& outputarray = output->getCube(0);
    const int lastzidx = outputarray.info().getSize(2) - 1;

    TypeSet<float> depths, extdepths;
    TypeSet<int> dfids, extdfids;
    BinID nearbid = bid;

    PtrMan<Gridder2D> gridder = gridder_->clone();
    gridder->setGridPoint( SI().transform(nearbid) );

    mAllocVarLenArr(float,vals,lastzidx+1);
    int lasthcidx=-1, firsthcidx=-1;
    for ( int idx=lastzidx; idx>=0; idx-- )
    {
	vals[idx] = mUdf(float);
	const float z = float( (output->z0_+idx) * output->zstep_ );
	const float layeridx = layermodel_->getLayerIndex( bid, z );

	TypeSet<Coord> wellposes;
	TypeSet<float> logvals;
	for ( int idy=0; idy<infos_.size(); idy++ )
	{
	    WellLogInfo* info = infos_[idy];
	    const Well::Data& wd = info->wd_;
	    const Well::Log* log = wd.logs().getLog( logname_ );
	    if ( !log ) continue;

	    TypeSet<float> mds = getMDs( *info, layeridx );
	    if ( mds.isEmpty() )
		continue;

	    for ( int idz=0; idz<mds.size(); idz++ )
	    {
		const float lv = log->getValue( mds[idz], extlog_ );
		if ( mIsUdf(lv) )
		    continue;

		const Coord pos = wd.track().getPos( mds[idz] );
		if ( mIsUdf(pos.x) || mIsUdf(pos.y) )
		    continue;

		wellposes += pos;
		logvals += lv;
	    }
	}

	gridder->setPoints( wellposes );
	gridder->setValues( logvals, false );
	if ( !gridder->init() )
	    continue;

	vals[idx] = gridder->getValue();
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

    IOPar lmpar;
    if ( getLayerModel(lmpar) )
	pars.mergeComp( lmpar, sKeyLayerModel() );
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
    const bool res = lmpar ? setLayerModel( *lmpar ) : false;
    return res;
}

} // namespace VolProc
