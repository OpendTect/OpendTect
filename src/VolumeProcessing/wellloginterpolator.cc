/*+
 *(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 *Author:	Y.C. Liu
 *Date:		April 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "wellloginterpolator.h"

#include "arraynd.h"
#include "gridder2d.h"
#include "survinfo.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltrack.h"


namespace VolProc
{

static const char* sKeyNrWells()	{ return "Nr of wells"; }
static const char* sKeyWellLogID()	{ return "WellLog ID"; }
static const char* sKeyLogName()	{ return "Log name"; }
static const char* sKeyAlgoName()	{ return "Algorithm"; }
static const char* sKeyExtension()	{ return "Output boundary"; }
static const char* sKeyLogExtension()	{ return "Logs Extension"; }


WellLogInterpolator::WellLogInterpolator()
    : gridder_( 0 )
    , extension_( ExtrapolateEdgeValue )
    , extlog_( 0 )
{}


WellLogInterpolator::~WellLogInterpolator()
{
    releaseData();
}


void WellLogInterpolator::releaseData()
{
    Step::releaseData();
    delete gridder_; gridder_ = 0;
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

    return true;
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
	/*
	vals[idx] = mUdf(float);
	const float z = (float) ( ( output->z0_ + idx ) * output->zstep_ );
	float horidx = computeHorizonIndex(depths,dfids,z);
	if ( horidx<0 && extension_==ExtrapolateEdgeValue )
	    horidx = computeHorizonIndex(extdepths,extdfids,z);
	if ( horidx < 0 )
	    continue;

	TypeSet<Coord> wellposes;
	TypeSet<float> logvals;
	for ( int idy=0; idy<info_.size(); idy++ )
	{
	    TypeSet<float> mds;
	    if ( !info_[idy]->getMDS(horidx,mds) )
		continue;

	    Well::Data* data = Well::MGR().get( *wellmids_[idy] );
	    if ( !data )
		continue;

	    for ( int idz=0; idz<mds.size(); idz++ )
	    {
		const float lv = data->logs().getLog( logidx_[idy] ).
		    getValue( mds[idz], extlog_ );
		if ( mIsUdf(lv) )
		    continue;

		const Coord pos = data->track().getPos( mds[idz] );
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
	*/
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

    return true;
}

} // namespace VolProc
