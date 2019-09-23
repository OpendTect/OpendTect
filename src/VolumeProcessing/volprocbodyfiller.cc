/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : November 2007
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
const char* BodyFiller::sKeyOldDBKey() { return "MarchingCubeSurface ID"; }
const char* BodyFiller::sKeyOldInsideOutsideValue()
					{ return "Surface InsideOutsideValue"; }

const char* BodyFiller::sKeyDBKey()		{ return "Body ID"; }
const char* BodyFiller::sKeyInsideType()	{ return "Inside Type"; }
const char* BodyFiller::sKeyOutsideType()	{ return "Outside Type"; }
const char* BodyFiller::sKeyInsideValue()	{ return "Inside Value"; }
const char* BodyFiller::sKeyOutsideValue()	{ return "Outside Value"; }


void BodyFiller::initClass()
{
    FileMultiString keys( BodyFiller::sFactoryKeyword() );
    keys += BodyFiller::sKeyOldType();

    FactoryType::Creator cr = createStepInstance;
    factory().addCreator( cr, keys.str(),
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


bool BodyFiller::setSurface( const DBKey& emid )
{
    if ( emobj_ )
    {
	emobj_->unRef();
	body_ = 0;
	emobj_ = 0;
    }

    SilentTaskRunnerProvider trprov;
    ConstRefMan<EM::Object> emobj = EM::BodyMan().fetch( emid, trprov );
    mDynamicCastGet( const EM::Body*, newsurf, emobj.ptr() );
    if ( !newsurf ) return false;

    emobj->ref();

    mid_ = emid;
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

    const TrcKeySampling ouths( output->horSubSel() );
    const StepInterval<float> zrg( output->zRange() );

    const int outputinlidx = ouths.inlRange().nearestIndex( bid.inl() );
    const int outputcrlidx = ouths.crlRange().nearestIndex( bid.crl() );
    const int outputzsz = output->nrZ();
    if ( outputinlidx<0 || outputcrlidx<0 )
	return true;

    if ( insidevaltype_ == Undefined ) insideval_ = mUdf(float);
    if ( insidevaltype_ == PrevStep ) insideval_ = -mUdf(float);
    if ( outsidevaltype_ == Undefined ) outsideval_ = mUdf(float);
    if ( outsidevaltype_ == PrevStep ) outsideval_ = -mUdf(float);

    const RegularSeisDataPack* input = getInput( getInputSlotID(0) );
    int inputinlidx = 0, inputcrlidx = 0, inputzsz = 0;
    bool useinput = false;
    if ( input )
    {
	const auto inlrg( input->horSubSel().lineNrRange() );
	const auto crlrg( input->horSubSel().trcNrRange() );
	inputinlidx = inlrg.nearestIndex( bid.inl() );
	inputcrlidx = crlrg.nearestIndex( bid.crl() );
	useinput =
	    inputinlidx>=0 && inputinlidx<input->data(0).getSize(0) &&
	    inputcrlidx>=0 && inputcrlidx<input->data(0).getSize(1);
	if ( useinput )
	    inputzsz = input->data(0).getSize(2);
    }

    int bodyinlidx = mUdf(int), bodycrlidx = mUdf(int);
    bool alloutside = true;
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
	    bodyinlidx>=implicitbody_->arr_->getSize(0) ||
	    bodycrlidx>=implicitbody_->arr_->getSize(1);
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
		}
		else
		{
		    const double curx = plgdir_==plgIsInline ?
			(double)bid.crl() : (double)bid.inl();
		    const double cury = plgdir_==plgIsZSlice ?
			(double)bid.crl() : scaledz;
		    isinside =
			polygon_->isInside( Coord(curx,cury), true, epsilon_ );
		}

		val = isinside ? insideval_ : outsideval_;
	    }
	    else
	    {
		const int bodyzidx =
			implicitbody_->tkzs_.zsamp_.nearestIndex(z);
		if ( bodyzidx<0 ||
			bodyzidx >= implicitbody_->arr_->getSize(2) )
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
    par.set( sKeyDBKey(), mid_ );
    par.set( sKeyInsideType(), insidevaltype_ );
    par.set( sKeyInsideValue(), insideval_ );
    par.set( sKeyOutsideType(), outsidevaltype_ );
    par.set( sKeyOutsideValue(), outsideval_ );
}


bool BodyFiller::usePar( const IOPar& par )
{
    if ( !Step::usePar(par) )
	return false;

    DBKey mid;
    if ( (!par.get(sKeyDBKey(),mid) && !par.get(sKeyOldDBKey(),mid) ) ||
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


ReportingTask* BodyFiller::createTask()
{
    RegularSeisDataPack* output = getOutput( getOutputSlotID(0) );
    if ( !output || !body_ )
	return 0;

    plgknots_.erase();
    plgbids_.erase();
    flatpolygon_.setEmpty();
    delete implicitbody_;
    implicitbody_ = body_->createImplicitBody( SilentTaskRunnerProvider(),
						false );

    if ( implicitbody_ )
	return Step::createTask();

    mDynamicCastGet( const EM::PolygonBody*, plg, body_ );
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
	plgknots_[idx].z_ *= SI().zScale();
	const BinID bid = SI().transform( plgknots_[idx].getXY() );
	plgbids_ += Coord3(bid.inl(),bid.crl(),plgknots_[idx].z_);

	if ( flatpolygon_.isEmpty() )
	{
	    flatpolygon_.hsamp_.start_ = flatpolygon_.hsamp_.stop_ = bid;
	    flatpolygon_.zsamp_.start = flatpolygon_.zsamp_.stop
				   = (float)plgknots_[idx].z_;
	}
	else
	{
	    flatpolygon_.hsamp_.include( bid );
	    flatpolygon_.zsamp_.include( (float) plgknots_[idx].z_ );
	}
    }

    if ( surf->nrPolygons()==1 )
    {
	if ( !flatpolygon_.hsamp_.inlRange().width() )
	{
	    plgdir_ = plgIsInline;
	    for ( int idx=0; idx<plgbids_.size(); idx++ )
	    {
		polygon_->add( Geom::Point2D<double>(plgbids_[idx].y_,
						     plgbids_[idx].z_) );
	    }
	}
	else if ( !flatpolygon_.hsamp_.crlRange().width() )
	{
	    plgdir_ = plgIsCrline;
	    for ( int idx=0; idx<plgbids_.size(); idx++ )
	    {
		polygon_->add( Geom::Point2D<double>(plgbids_[idx].x_,
						     plgbids_[idx].z_) );
	    }
	}
	else
	{
	    plgdir_ = plgIsZSlice;
	    for ( int idx=0; idx<plgbids_.size(); idx++ )
	    {
		polygon_->add( Geom::Point2D<double>(plgbids_[idx].x_,
						     plgbids_[idx].y_) );
	    }
	}

	delete polygon_;
	polygon_ = new ODPolygon<double>();
	for ( int idx=0; idx<plgbids_.size(); idx++ )
	{
	    const double curx = plgdir_==plgIsInline ? plgbids_[idx].y_
						     : plgbids_[idx].x_;
	    const double cury = plgdir_==plgIsZSlice ? plgbids_[idx].y_
						     : plgbids_[idx].z_;
	    polygon_->add( Coord(curx,cury) );
	}
    }
    else
	plgdir_ = plgIsOther;

    epsilon_ = plgdir_<2 ? plgbids_[0].distTo<float>(plgbids_[1])*0.01
			 : plgknots_[0].distTo<float>(plgknots_[1])*0.01;

    return Step::createTask();
}


od_int64 BodyFiller::extraMemoryUsage( OutputSlotID,
				       const TrcKeyZSampling& tkzs ) const
{
    return implicitbody_ ? getComponentMemory(implicitbody_->tkzs_,false) * 2
			 : 0;
}


} // namespace VolProc
