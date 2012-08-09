/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : November 2007
-*/

static const char* rcsID mUnusedVar = "$Id: volprocbodyfiller.cc,v 1.16 2012-08-09 03:43:21 cvssalil Exp $";

#include "volprocbodyfiller.h"

#include "emmarchingcubessurface.h"
#include "emmanager.h"
#include "empolygonbody.h"
#include "iopar.h"
#include "embody.h"
#include "rowcol.h"
#include "separstr.h"
#include "survinfo.h"
#include "trigonometry.h"

namespace VolProc
{

#define plgIsInline	0
#define plgIsCrline	1
#define plgIsZSlice	2
#define plgIsOther	3

void BodyFiller::initClass()
{
    SeparString keys( BodyFiller::sFactoryKeyword(), VolProc::Step::factory().cSeparator() );
    keys += BodyFiller::sKeyOldType();

    VolProc::Step::factory().addCreator( createInstance, keys, BodyFiller::sFactoryDisplayName() );
}    


BodyFiller::BodyFiller()
    : body_( 0 )  
    , emobj_( 0 )
    , implicitbody_( 0 )
    , insideval_( mUdf(float) )
    , outsideval_( mUdf(float) )
    , flatpolygon_( false )
    , plgdir_( 0 )
    , epsilon_( 1e-4 )
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


void BodyFiller::setInsideOutsideValue( const float in, const float out )
{
    insideval_ = in;
    outsideval_ = out;
}


bool BodyFiller::computeBinID( const BinID& bid, int )
{
    //ToDo: Interpolate values from Implicit version of Surface(array).
    if ( !output_ || !output_->nrCubes() )
	return false;

    const bool flatbody = !implicitbody_;
    if ( flatbody && plgknots_.size()<2 )
	return false;
   
    const int outputinlidx = output_->inlsampling_.nearestIndex( bid.inl );
    const int outputcrlidx = output_->crlsampling_.nearestIndex( bid.crl );
    const int outputzsz = output_->getZSz();
    if ( outputinlidx<0 || outputcrlidx<0 )
	return true;

    const int inputinlidx = input_ 
	? input_->inlsampling_.nearestIndex(bid.inl) : 0;
    const int inputcrlidx = input_
	? input_->crlsampling_.nearestIndex(bid.crl) : 0;
    const bool useinput = input_ &&
	inputinlidx>=0 && inputinlidx < input_->getCube(0).info().getSize(0) &&
	inputcrlidx>=0 && inputcrlidx < input_->getCube(0).info().getSize(1);
    const int inputzsz = useinput && input_
	? input_->getCube(0).info().getSize(2) : 0;
 
    int bodyinlidx, bodycrlidx;
    bool alloutside;
    Interval<double> plgzrg( mUdf(double), mUdf(double) );
    if ( flatbody )
    {
	alloutside = !flatpolygon_.hrg.inlRange().includes(bid.inl,false) ||
		     !flatpolygon_.hrg.crlRange().includes(bid.crl,false);
	if ( !alloutside )
	{
	    if ( !getFlatPlgZRange( bid, plgzrg ) )
		alloutside = true;
	}
    }
    else
    {
	bodyinlidx = implicitbody_->cs_.hrg.inlRange().nearestIndex( bid.inl );
	bodycrlidx = implicitbody_->cs_.hrg.crlRange().nearestIndex( bid.crl );
	
	alloutside = bodyinlidx<0 || bodycrlidx<0 ||
	    bodyinlidx>=implicitbody_->arr_->info().getSize(0) ||
	    bodycrlidx>=implicitbody_->arr_->info().getSize(1);
    }

    for ( int idx=0; idx<outputzsz; idx++ )
    {
	float val;
	if ( alloutside )
	    val = outsideval_;
	else
	{
	    const float z = (float) ( (output_->z0_+idx) * output_->zstep_ );
	    if ( flatbody )
		val = plgzrg.includes( z * SI().zScale(), true ) ? insideval_
							   : outsideval_;
	    else
	    {
    		const int bodyzidx = implicitbody_->cs_.zrg.nearestIndex(z);
    		if ( bodyzidx<0 || 
			bodyzidx>=implicitbody_->arr_->info().getSize(2) )
    		    val = outsideval_;
    		else
    		{
    		    const float bodyval = implicitbody_->arr_->get( 
			    bodyinlidx, bodycrlidx, bodyzidx);
		    
    		    if ( mIsUdf(bodyval) )
    			val = mUdf(float);
    		    else
    			val = bodyval>implicitbody_->threshold_
    			    ? insideval_ : outsideval_;
    		}
	    }
	}
	   
	if ( mIsUdf(val) && useinput )
	{
	    const int inputidx = idx+output_->z0_-input_->z0_;
	    if ( inputidx>=0 && inputidx<inputzsz )
		val = input_->getCube(0).get(inputinlidx,inputcrlidx,inputidx);
	}

	output_->setValue( 0, outputinlidx, outputcrlidx, idx, val );
    }

    return true;
}


bool BodyFiller::needsInput() const
{ return true; }


void BodyFiller::setOutput( Attrib::DataCubes* ni )
{
    if ( output_ ) 
	output_->unRef();

    output_ = ni;
    if ( output_ ) output_->ref();
}


void BodyFiller::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
    par.set( sKeyMultiID(), mid_ );
    par.set( sKeyInsideOutsideValue(), insideval_, outsideval_ );
    par.setYN( sKeyEnabled(), enabled_ );
}


bool BodyFiller::usePar( const IOPar& par )
{
    if ( !Step::usePar( par ) )
	return false;

    MultiID mid;
    if ( (!par.get(sKeyMultiID(), mid) && !par.get(sKeyOldMultiID(), mid ) ) ||
          !setSurface( mid ) )
	return false;

    float inv, outv;
    if ( par.get(sKeyInsideOutsideValue(), inv, outv ) )
	setInsideOutsideValue( inv, outv);

    if ( !par.getYN(sKeyEnabled(), enabled_) )
	return false;

    return true;
}


Task* BodyFiller::createTask()
{
    if ( !output_ || !body_ )
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
    
    const EM::SectionID sid = plg->sectionID( 0 );
    const Geometry::PolygonSurface* surf = plg->geometry().sectionGeometry(sid);
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
	plgbids_ += Coord3(bid.inl,bid.crl,plgknots_[idx].z);
	
	if ( flatpolygon_.isEmpty() )
	{
	    flatpolygon_.hrg.start = flatpolygon_.hrg.stop = bid;
	    flatpolygon_.zrg.start = flatpolygon_.zrg.stop = (float) plgknots_[idx].z;
	}
	else
	{
	    flatpolygon_.hrg.include( bid );
	    flatpolygon_.zrg.include( (float) plgknots_[idx].z );
	}
    }
    
    if ( surf->nrPolygons()==1 )
    {
	if ( !flatpolygon_.hrg.inlRange().width() )
	    plgdir_ = plgIsInline;
	else if ( !flatpolygon_.hrg.crlRange().width() )
	    plgdir_ = plgIsCrline;
	else
	    plgdir_ = plgIsZSlice;
    }
    else
	plgdir_ = plgIsOther;	

    epsilon_ = plgdir_ < 2 ? plgbids_[0].distTo(plgbids_[1])*0.01 
			  : plgknots_[0].distTo(plgknots_[1])*0.01;
    
    return Step::createTask();    
}


bool BodyFiller::getFlatPlgZRange( const BinID& bid, Interval<double>& res )
{
    const Coord coord = SI().transform( bid );
    if ( plgdir_ < 2 ) //Inline or Crossline case
    {
	int count = 0;
	double z = flatpolygon_.zrg.start;
	while ( z <= flatpolygon_.zrg.stop )
	{
	    if ( pointInPolygon( Coord3(coord,z), plgbids_, epsilon_ ) )
	    {
		if ( !count )
		    res.start = res.stop = z;
		else
		    res.include( z );
		
		count++;
	    }
	    
	    z += output_->zstep_ * SI().zScale();
	}
	
	if ( count==1 )
	{
	    res.start -= 0.5 * output_->zstep_;
	    res.stop += 0.5 * output_->zstep_;
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
		res.start -= 0.5 * output_->zstep_;
		res.stop += 0.5 * output_->zstep_;
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
	    res.start = z - 0.5 * output_->zstep_;
	    res.stop = z + 0.5 * output_->zstep_;
	}
    }

    return true;
}



}; //namespace
