/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volprocbodyfiller.h"

#include "emmarchingcubessurface.h"
#include "emmanager.h"
#include "empolygonbody.h"
#include "executor.h"
#include "iopar.h"
#include "embody.h"
#include "polygon.h"
#include "rowcol.h"
#include "seisdatapack.h"
#include "separstr.h"
#include "survinfo.h"
#include "trigonometry.h"


namespace VolProc
{

#define plgIsInline	0
#define plgIsCrline	1
#define plgIsZSlice	2
#define plgIsOther	3


const char* BodyFiller::sKeyOldType()	{ return "MarchingCubes"; }
const char* BodyFiller::sKeyOldMultiID() { return "MarchingCubeSurface ID"; }
const char* BodyFiller::sKeyOldInsideOutsideValue()
					{ return "Surface InsideOutsideValue"; }

const char* BodyFiller::sKeyMultiID()		{ return "Body ID"; }
const char* BodyFiller::sKeyInsideType()	{ return "Inside Type"; }
const char* BodyFiller::sKeyOutsideType()	{ return "Outside Type"; }
const char* BodyFiller::sKeyInsideValue()	{ return "Inside Value"; }
const char* BodyFiller::sKeyOutsideValue()	{ return "Outside Value"; }


void BodyFiller::initClass()
{
    SeparString keys( BodyFiller::sFactoryKeyword(), factory().cSeparator() );
    keys += BodyFiller::sKeyOldType();

    factory().addCreator( createInstance, keys,
			  BodyFiller::sFactoryDisplayName() );
}


BodyFiller::BodyFiller()
    : body_(0)
    , emobj_(0)
    , implicitbody_(0)
    , insidevaltype_(Constant)
    , outsidevaltype_(Constant)
    , insideval_(mUdf(float))
    , outsideval_(mUdf(float))
    , flatpolygon_(false)
    , plgdir_(0)
    , epsilon_(1e-4)
    , polygon_(0)
{}


BodyFiller::~BodyFiller()
{
    releaseData();
}


void BodyFiller::releaseData()
{
    Step::releaseData();

    if ( emobj_ ) emobj_->unRef();
    emobj_ = 0;

    delete implicitbody_;
    implicitbody_ = 0;

    plgknots_.erase();
    plgbids_.erase();

    delete polygon_; polygon_ = 0;
}


bool BodyFiller::setSurface( const MultiID& mid )
{
    if ( emobj_ )
    {
	emobj_->unRef();
	body_ = 0;
	emobj_ = 0;
    }

    EM::ObjectID emid = EM::EMM().getObjectID( mid );
    RefMan<EM::EMObject> emobj = EM::EMM().getObject( emid );
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	PtrMan<Executor> loader = EM::EMM().objectLoader( mid );
	if ( !loader || !loader->execute() )
	    return false;

	emid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( emid );
    }

    mDynamicCastGet( EM::Body*, newsurf, emobj.ptr() );
    if ( !newsurf ) return false;

    emobj->ref();

    mid_ = mid;
    body_ = newsurf;
    emobj_ = emobj;
    emobj = 0;

    delete implicitbody_;
    implicitbody_ = 0;

    return true;
}


void BodyFiller::setInsideValueType( ValueType vt )
{ insidevaltype_ = vt ; }

BodyFiller::ValueType BodyFiller::getInsideValueType() const
{ return insidevaltype_; }

void BodyFiller::setOutsideValueType( ValueType vt )
{ outsidevaltype_ = vt; }

BodyFiller::ValueType BodyFiller::getOutsideValueType() const
{ return outsidevaltype_; }

void BodyFiller::setInsideValue( float val )
{ insideval_ = val; }

float BodyFiller::getInsideValue() const
{ return insideval_; }

void BodyFiller::setOutsideValue( float val )
{ outsideval_ = val; }

float BodyFiller::getOutsideValue() const
{ return outsideval_; }


bool BodyFiller::computeBinID( const BinID& bid, int )
{
    //ToDo: Interpolate values from Implicit version of Surface(array).
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || output->isEmpty() )
	return false;

    const bool flatbody = !implicitbody_;
    if ( flatbody && plgknots_.size()<2 )
	return false;

    const TrcKeySampling& ouths = output->sampling().hsamp_;
    const StepInterval<float>& zrg( output->sampling().zsamp_ );

    const int outputinlidx = ouths.inlRange().nearestIndex( bid.inl() );
    const int outputcrlidx = ouths.crlRange().nearestIndex( bid.crl() );
    const int outputzsz = output->sampling().nrZ();
    if ( outputinlidx<0 || outputcrlidx<0 )
	return true;

    if ( insidevaltype_ == Undefined ) insideval_ = mUdf(float);
    if ( insidevaltype_ == PrevStep ) insideval_ = -mUdf(float);
    if ( outsidevaltype_ == Undefined ) outsideval_ = mUdf(float);
    if ( outsidevaltype_ == PrevStep ) outsideval_ = -mUdf(float);

    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    const int inputinlidx = input
	? input->sampling().hsamp_.inlRange().nearestIndex(bid.inl()) : 0;
    const int inputcrlidx = input
	? input->sampling().hsamp_.crlRange().nearestIndex(bid.crl()) : 0;
    const bool useinput = input &&
	inputinlidx>=0 && inputinlidx<input->data(0).info().getSize(0) &&
	inputcrlidx>=0 && inputcrlidx<input->data(0).info().getSize(1);
    const int inputzsz = useinput && input
	? input->data(0).info().getSize(2) : 0;

    int bodyinlidx = mUdf(int), bodycrlidx = mUdf(int);
    bool alloutside;
    if ( flatbody )
    {
	alloutside = !flatpolygon_.hsamp_.inlRange().includes(bid.inl(),false)
		  || !flatpolygon_.hsamp_.crlRange().includes(bid.crl(),false);
    }
    else
    {
	bodyinlidx =
	    implicitbody_->tkzs_.hsamp_.inlRange().nearestIndex( bid.inl() );
	bodycrlidx =
	    implicitbody_->tkzs_.hsamp_.crlRange().nearestIndex( bid.crl() );

	alloutside = bodyinlidx<0 || bodycrlidx<0 ||
	    bodyinlidx>=implicitbody_->arr_->info().getSize(0) ||
	    bodycrlidx>=implicitbody_->arr_->info().getSize(1);
    }

    for ( int zidx=0; zidx<outputzsz; zidx++ )
    {
	float val;
	if ( alloutside )
	    val = outsideval_;
	else
	{
	    const float z = zrg.atIndex( zidx );
	    if ( flatbody )
	    {
		const float scaledz = z * SI().zScale();
		bool isinside = false;
		if ( plgdir_ == plgIsOther )
		{
		    Coord3 curpos((double)bid.inl(),(double)bid.crl(),scaledz);
		    isinside = pointInPolygon( curpos, plgbids_, epsilon_ );
		    /*If polygon is not convex, this will not work*/
		}
		else
		{
		    const double curx = plgdir_==plgIsInline ?
			(double)bid.crl() : (double)bid.inl();
		    const double cury = plgdir_==plgIsZSlice ?
			(double)bid.crl() : scaledz;
		    isinside = polygon_->isInside( Coord(curx,cury),
						   true, epsilon_ );
		}

		val = isinside ? insideval_ : outsideval_;
	    }
	    else
	    {
		const int bodyzidx =
			implicitbody_->tkzs_.zsamp_.nearestIndex(z);
		if ( bodyzidx<0 ||
		     bodyzidx >= implicitbody_->arr_->info().getSize(2) )
		    val = outsideval_;
		else
		{
		    const float bodyval = implicitbody_->arr_->get(
			    bodyinlidx, bodycrlidx, bodyzidx );

		    if ( mIsUdf(bodyval) )
			val = mUdf(float);
		    else
			val = bodyval >= implicitbody_->threshold_
			    ? insideval_ : outsideval_;
		}
	    }
	}

	if ( mIsUdf(-val) && useinput )
	{
	    const int inputidx = zidx;
	    if ( inputidx>=0 && inputidx<inputzsz )
		val = input->data(0).get(inputinlidx,inputcrlidx,inputidx);
	}

	output->data(0).set( outputinlidx, outputcrlidx, zidx, val );
    }

    return true;
}


void BodyFiller::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
    par.set( sKeyMultiID(), mid_ );
    par.set( sKeyInsideType(), insidevaltype_ );
    par.set( sKeyInsideValue(), insideval_ );
    par.set( sKeyOutsideType(), outsidevaltype_ );
    par.set( sKeyOutsideValue(), outsideval_ );
}


bool BodyFiller::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    MultiID mid;
    if ( (!par.get(sKeyMultiID(),mid) && !par.get(sKeyOldMultiID(),mid) ) ||
	 !setSurface(mid) )
	return false;

    insidevaltype_ = outsidevaltype_ = Constant;
    insideval_ = outsideval_ = mUdf(float);
    if ( par.get(sKeyOldInsideOutsideValue(),insideval_,outsideval_) )
    {
	if ( mIsUdf(insideval_) ) insidevaltype_ = PrevStep;
	if ( mIsUdf(outsideval_) ) outsidevaltype_ = PrevStep;
	return true;
    }

    int val = 0;
    par.get( sKeyInsideType(), val ); insidevaltype_ = (ValueType)val;
    par.get( sKeyOutsideType(), val ); outsidevaltype_ = (ValueType)val;
    par.get( sKeyInsideValue(), insideval_ );
    par.get( sKeyOutsideValue(), outsideval_ );

    return true;
}


Task* BodyFiller::createTask()
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || !body_ )
	return 0;

    plgknots_.erase();
    plgbids_.erase();
    flatpolygon_.setEmpty();
    delete implicitbody_;
    implicitbody_ = body_->createImplicitBody( 0, false );

    if ( implicitbody_ )
	return Step::createTask();

    mDynamicCastGet( EM::PolygonBody*, plg, body_ );
    if ( !plg )
	return 0;

    const Geometry::PolygonSurface* surf = plg->geometry().geometryElement();
    if ( surf->nrPolygons()==1 )
	surf->getCubicBezierCurve( 0, plgknots_, SI().zScale() );
    else
	surf->getAllKnots( plgknots_ );

    const int knotsz = plgknots_.size();
    if ( knotsz<2 )
	return 0;

    for ( int idx=0; idx<knotsz; idx++ )
    {
	plgknots_[idx].z *= SI().zScale();
	const BinID bid = SI().transform( plgknots_[idx] );
	plgbids_ += Coord3(bid.inl(),bid.crl(),plgknots_[idx].z);

	if ( flatpolygon_.isEmpty() )
	{
	    flatpolygon_.hsamp_.start_ = flatpolygon_.hsamp_.stop_ = bid;
	    flatpolygon_.zsamp_.start = flatpolygon_.zsamp_.stop
				   = (float)plgknots_[idx].z;
	}
	else
	{
	    flatpolygon_.hsamp_.include( bid );
	    flatpolygon_.zsamp_.include( (float) plgknots_[idx].z );
	}
    }

    if ( surf->nrPolygons()==1 )
    {
	if ( !flatpolygon_.hsamp_.inlRange().width() )
	    plgdir_ = plgIsInline;
	else if ( !flatpolygon_.hsamp_.crlRange().width() )
	    plgdir_ = plgIsCrline;
	else
	    plgdir_ = plgIsZSlice;

	ODPolygon<double>* newplg = new ODPolygon<double>();
	for ( int idx=0; idx<plgbids_.size(); idx++ )
	{
	    const double curx = plgdir_==plgIsInline ? plgbids_[idx].y
						     : plgbids_[idx].x;
	    const double cury = plgdir_==plgIsZSlice ? plgbids_[idx].y
						     : plgbids_[idx].z;
	    newplg->add( Coord(curx,cury) );
	}

	delete polygon_;
	polygon_ = newplg;
    }
    else
	plgdir_ = plgIsOther;

    epsilon_ = plgdir_<2 ? plgbids_[0].distTo(plgbids_[1])*0.01
			 : plgknots_[0].distTo(plgknots_[1])*0.01;

    return Step::createTask();
}


bool BodyFiller::getFlatPlgZRange( const BinID& bid, Interval<double>& res )
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output) return false;

    const float zstep = output->sampling().zsamp_.step;
    const Coord coord = SI().transform( bid );
    if ( plgdir_ < 2 ) //Inline or Crossline case
    {
	int count = 0;
	double z = flatpolygon_.zsamp_.start;
	while ( z <= flatpolygon_.zsamp_.stop )
	{
	    if ( pointInPolygon( Coord3(coord,z), plgbids_, epsilon_ ) )
	    {
		if ( !count )
		    res.start = res.stop = z;
		else
		    res.include( z );

		count++;
	    }

	    z += zstep * SI().zScale();
	}

	if ( count==1 )
	{
	    res.start -= 0.5 * zstep;
	    res.stop += 0.5 * zstep;
	}

	return count;
    }
    else
    {
	TypeSet<Coord3> knots;
	for ( int idx=0; idx<plgknots_.size(); idx++ )
	    knots += Coord3( plgknots_[idx].x, plgknots_[idx].y, 0 );

	if ( !pointInPolygon( Coord3(coord,0), knots, epsilon_ ) )
	    return false;

	if ( plgdir_==plgIsZSlice )
	{
	    for ( int idx=0; idx<plgknots_.size(); idx++ )
	    {
		if ( !idx )
		    res.start = res.stop = plgknots_[0].z;
		else
		    res.include( plgknots_[idx].z );
	    }

	    if ( mIsZero( res.width(), 1e-3 ) )
	    {
		res.start -= 0.5 * zstep;
		res.stop += 0.5 * zstep;
	    }
	}
	else //It is a case hard to see on the display.
	{
	    const Coord3 normal = (plgknots_[1] - plgknots_[0]).cross(
		    plgknots_[2] - plgknots_[1] );
	    if ( mIsZero(normal.z, 1e-4) ) //Should not happen case
		return false;

	    const Coord diff = coord - plgknots_[0].coord();
	    const double z = plgknots_[0].z -
		( normal.x * diff.x + normal.y * diff.y ) / normal.z;
	    res.start = z - 0.5 * zstep;
	    res.stop = z + 0.5 * zstep;
	}
    }

    return true;
}


od_int64 BodyFiller::extraMemoryUsage( OutputSlotID,
	const TrcKeySampling& hsamp, const StepInterval<int>& ) const
{
    if ( !implicitbody_ ) return 0;

    const TrcKeyZSampling bodycs = implicitbody_->tkzs_;

    return getComponentMemory( bodycs.hsamp_, false ) * 2;
}


} // namespace VolProc
