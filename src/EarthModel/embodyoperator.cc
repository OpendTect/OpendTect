/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embodyoperator.cc,v 1.15 2011-05-06 17:49:16 cvsyuancheng Exp $";

#include "embodyoperator.h"

#include "array3dfloodfill.h"
#include "arrayndimpl.h"
#include "delaunay3d.h"
#include "embody.h"
#include "embodytr.h"
#include "emmanager.h"
#include "empolygonbody.h"
#include "emmarchingcubessurface.h"
#include "emrandomposbody.h"
#include "mousecursor.h"
#include "survinfo.h"
#include "task.h"
#include "ioman.h"
#include "ioobj.h"
#include "executor.h"

namespace EM
{

class BodyOperatorArrayFiller: public ParallelTask
{
public:
BodyOperatorArrayFiller( const ImplicitBody& b0, const ImplicitBody& b1, 
	const StepInterval<int>& inlrg, const StepInterval<int>& crlrg, 
	const StepInterval<float>& zrg,	BodyOperator::Action act, 
	Array3D<float>& arr ) 
    : arr_( arr )
    , b0_( b0 )
    , b1_( b1 )
    , inlrg_( inlrg )	       
    , crlrg_( crlrg )	       
    , zrg_( zrg )
    , action_( act )		 
{}

od_int64 nrIterations() const { return arr_.info().getTotalSz(); }
		       
protected:

bool doWork( od_int64 start, od_int64 stop, int threadid )    
{
    int p[3], id0[3], id1[3];
    bool incube[2];
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	arr_.info().getArrayPos( idx, p );
	
	id0[0] = b0_.inlsampling_.nearestIndex( inlrg_.atIndex(p[0]) );
	id0[1] = b0_.crlsampling_.nearestIndex( crlrg_.atIndex(p[1]) );
	id0[2] = b0_.zsampling_.nearestIndex( zrg_.atIndex(p[2]) );
	id1[0] = b1_.inlsampling_.nearestIndex( inlrg_.atIndex(p[0]) );
	id1[1] = b1_.crlsampling_.nearestIndex( crlrg_.atIndex(p[1]) );
	id1[2] = b1_.zsampling_.nearestIndex( zrg_.atIndex(p[2]) );
	
	incube[0] = b0_.arr_->info().validPos(id0[0], id0[1], id0[2]);
	incube[1] = b1_.arr_->info().validPos(id1[0], id1[1], id1[2]);

	char pos0 = cOutsde(), pos1 = cOutsde();
	float v0 = mUdf(float), v1 = mUdf(float);
	if ( incube[0] )
	{
	    v0 = b0_.arr_->get( id0[0], id0[1], id0[2] );
	    pos0 = (v0<b0_.threshold_) ? cInside() 
		 : (v0>b0_.threshold_ ? cOutsde() : cOnbody());
	}

	if ( incube[1] )
	{
	    v1 = b1_.arr_->get( id1[0], id1[1], id1[2] );
	    pos1 = (v1<b1_.threshold_) ? cInside() 
		 : (v1>b1_.threshold_ ? cOutsde() : cOnbody());
	}
		
	const float val = getVal( incube[0], incube[1], pos0, pos1, v0, v1 );
	arr_.set( p[0], p[1], p[2], val );
    }

    return true;
}


float getVal( bool incube0, bool incube1, char p0, char p1, float v0, 
	      float v1 ) const
{
    if ( !incube0 || !incube1 )
    {
	if ( incube0 )
	    return action_==BodyOperator::IntSect ? fabs(v0) : v0;
	else if ( incube1 )
	    return action_==BodyOperator::Union ? v1 : fabs(v1);
	else
	    return 1;
    }

    float v[2] = { fabs(v0), fabs(v1) };
    if ( action_==BodyOperator::Union )
    {
	if ( p0==cInside() )
	    return p1==cInside() ? -mMAX(v[0],v[1]) : -v[0];
	else if ( p1==cInside() )
	    return p0==cInside() ? -mMAX(v[0],v[1]) : -v[1];
	else if ( p0==cOnbody() || p1==cOnbody() )
	    return 0;
	else
	    return mMIN(v[0],v[1]);

	/* Binary sign.
	return (p0==cInside() || p1==cInside()) ? cInside() :
	       ( (p0==cOnbody() || p1==cOnbody()) ? cOnbody() : cOutsde() );*/

    }
    else if ( action_==BodyOperator::IntSect )
    {
	if ( p0==cInside() && p1==cInside() )
	    return -mMIN(v[0],v[1]);
	else if ( (p0==cOnbody() && p1==cInside()) || 
		  (p0==cInside() && p1==cOnbody()) || 
		  (p0==cOnbody() && p1==cOnbody()) )
	    return 0;
	else
	    return mMAX(v[0],v[1]);

	/* Binary sign.
	return (p0==cInside() && p1==cInside()) ? cInside() : 
	    ( ((p0==cOnbody() && p1==cInside()) || 
	       (p0==cInside() && p1==cOnbody()) || 
	       (p0==cOnbody() && p1==cOnbody())) ? cOnbody() : cOutsde() );*/
    }
    else 
    {
	if ( p0==cOutsde() ) 
	    return p1==cInside() ? mMAX(v[0],v[1]) : v[0];
	else if ( p1==cInside() ) 
	    return mMAX(v[0],v[1]);
	else if ( p0==cInside() && p1==cOutsde() ) 
	    return -mMIN(v[0],v[1]);
	else
    	    return 0;

	/* Binary sign.
	return (p0==cOutsde() || p1==cInside()) ? cOutsde() : 
	    ( (p0==cInside() && p1==cOutsde()) ? cInside() : cOnbody() );*/
    }
}

    static char cOnbody()	{ return 0; }
    static char cOutsde()	{ return 1; }
    static char cInside()	{ return -1; }

    const ImplicitBody&		b0_;
    const ImplicitBody&		b1_;
    Array3D<float>&		arr_;
    const StepInterval<int>&	inlrg_;
    const StepInterval<int>&	crlrg_;
    StepInterval<float>		zrg_;
    BodyOperator::Action	action_;
};


Expl2ImplBodyExtracter::Expl2ImplBodyExtracter( const DAGTetrahedraTree& tree,
	const StepInterval<int>& inlrg,	const StepInterval<int>& crlrg,
	const StepInterval<float>& zrg,	Array3D<float>& arr )
    : tree_( tree )
    , arr_( arr )
    , inlrg_( inlrg )
    , crlrg_( crlrg )
    , zrg_( zrg )
{
    zrg_.scale( SI().zScale() );
}


bool Expl2ImplBodyExtracter::doPrepare( int nrthreads )
{
    const TypeSet<Coord3>& crds = tree_.coordList();
    tree_.getSurfaceTriangles( tri_ );
    for ( int idx=0; idx<tri_.size()/3; idx++ )
    {
	const int startid = 3 * idx;
	planes_ += Plane3( crds[tri_[startid]], crds[tri_[startid+1]], 
			   crds[tri_[startid+2]] );
    }

    return planes_.size();
}


od_int64 Expl2ImplBodyExtracter::nrIterations() const
{ return arr_.info().getSize(0)*arr_.info().getSize(1); }


#define mSetSegment() \
if ( !found ) \
{ \
    segment.start = segment.stop = pos.z; \
    found = true; \
} \
else \
    segment.include( pos.z )


bool Expl2ImplBodyExtracter::doWork( od_int64 start, od_int64 stop, int )
{
    const TypeSet<Coord3>& crds = tree_.coordList();
    const int crlsz = arr_.info().getSize(1);
    const int zsz = arr_.info().getSize(2);
    const int planesize = planes_.size();

    for ( int idx=start; idx<=stop && shouldContinue(); idx++, addToNrDone(1) )
    {
	const int inlidx = idx / crlsz;
	const int crlidx = idx % crlsz;
	const BinID bid( inlrg_.atIndex(inlidx), crlrg_.atIndex(crlidx) );
	Coord3 pos( SI().transform(bid), 0 );
	Line3 vtln( pos, Coord3(0,0,1) );

	Interval<float> segment;
	bool found = false;
	for ( int pl=0; pl<planesize; pl++ )
	{
	    Coord3 v[3];
	    for ( int pidx=0; pidx<3; pidx++ )
		v[pidx] = crds[tri_[3*pl+pidx]];

	    const float fv = planes_[pl].A_*pos.x + planes_[pl].B_*pos.y +
		planes_[pl].D_;
	    if ( mIsZero(planes_[pl].C_,1e-8) ) 
	    {
		if ( mIsZero(fv,1e-8) )
		{ 
		    for ( int pidx=0; pidx<3; pidx++ )
		    {
			Line3 edge( v[pidx], v[(pidx+1)%3]-v[pidx] );
			double te, tv;
			vtln.closestPoint(edge,tv,te);
			if ( te<=1 && te>=0 )
			{
			    pos.z = v[pidx].z+te*(v[(pidx+1)%3].z-v[pidx].z);
			    mSetSegment();
			}
		    }
		}

		continue;
	    }
	    else
		pos.z = -fv/planes_[pl].C_;
	
	    if ( !pointInTriangle3D(pos,v[0],v[1],v[2],0) )
	    {
		if ( pointOnEdge3D( pos, v[0], v[1], 1e-3 ) || 
		     pointOnEdge3D( pos, v[1], v[2], 1e-3 ) || 
		     pointOnEdge3D( pos, v[2], v[0], 1e-3 ) )
		{
		    mSetSegment();
		}
	    }
	    else
	    {
		mSetSegment();
	    }
	}
	
	if ( found )
	{
    	    for ( int zidx=0; zidx<zsz; zidx++ )
    	    {
    		const float curz = zrg_.atIndex( zidx );
		const float val = curz<segment.start ? curz-segment.start :
		    ( curz>segment.stop ? segment.stop-curz : 
		      mMIN(curz-segment.start, segment.stop-curz) );		
		arr_.set( inlidx, crlidx, zidx, val );
	    }
	}
	else 
	{
    	    for ( int zidx=0; zidx<zsz; zidx++ )
		arr_.set( inlidx, crlidx, zidx, -1 );
	}
    }

    return true;
}



/*//Go by each point: too slow
 * bool Expl2ImplBodyExtracter::doWork( od_int64 start, od_int64 stop, int )
{
    int p[3]; BinID bid; Coord3 pos;
    const TypeSet<Coord3>& crds = tree_.coordList();
    const int planesize = planes_.size();

    for ( int idx=start; idx<=stop && shouldContinue(); idx++, addToNrDone(1) )
    {
	arr_.info().getArrayPos( idx, p );
	bid = BinID( inlrg_.atIndex(p[0]), crlrg_.atIndex(p[1]) );
	pos = Coord3( SI().transform(bid), zrg_.atIndex(p[2]) );

	bool isinside = planes_[0].onSameSide( pos, bodycenter_);;
	float mindist = -1;
	for ( int pl=0; pl<planesize; pl++ )
	{
	    const Coord3 proj = planes_[pl].getProjection( pos );
	    const float dist = (proj-pos).abs();

	    if ( dist<1e-4 )
	    {
		if ( !pointInTriangle3D( proj, crds[tri_[3*pl]],
			    crds[tri_[3*pl+1]],	crds[tri_[3*pl+2]], 1e-8) )
		    continue;

		if ( isinside )
		{
		    for ( int pidx=pl+1; pidx<planesize; pidx++ )
		    {
			if ( !isinside )
			    break;
			isinside = planes_[pidx].onSameSide(pos,bodycenter_);
		    } 
		}

		mindist = dist;
		break;
	    }

	    if ( mindist<0 ) 
	    {
		if ( isinside )
    		    isinside = planes_[pl].onSameSide(pos,bodycenter_); 
		mindist = dist;
	    }
	    else
	    {
		if ( isinside )
    		    isinside = planes_[pl].onSameSide(pos,bodycenter_); 

		if ( mindist>dist )
		    mindist = dist;
	    }
	}

	arr_.set( p[0], p[1], p[2], isinside ? mindist : -mindist );
    }

    return true;
} */


BodyOperator::BodyOperator()
    : inputbodyop0_( 0 )
    , inputbodyop1_( 0 )
    , inputbody0_( 0 )
    , inputbody1_( 0 )
    , id_( getFreeID() )
    , action_( Union )			       
{}


BodyOperator::~BodyOperator()
{
    delete inputbodyop0_;
    delete inputbodyop1_;
}


int BodyOperator::getFreeID()
{
    static int id = 0;
    return id++;
}


bool BodyOperator::isOK() const
{
    if ( inputbodyop0_ )
    {
	if ( !inputbodyop0_->isOK() )
	    return false;
    }
    else
    {
	if ( !IOM().get(inputbody0_) )
	    return false;
    }

    if ( inputbodyop1_ )
    {
	if ( !inputbodyop1_->isOK() )
	    return false;
    }
    else
    {
	if ( !IOM().get(inputbody1_) )
	    return false;
    }

    return true;
}


void BodyOperator::setInput( bool body0, const MultiID& mid )
{
    if ( body0 )
    {
	if ( inputbodyop0_ ) delete inputbodyop0_;
	inputbodyop0_ = 0;
	inputbody0_ = mid;
    }
    else
    {
	if ( inputbodyop1_ ) delete inputbodyop1_;
	inputbodyop1_ = 0;
	inputbody1_ = mid;
    }
}


void BodyOperator::setInput( bool body0, BodyOperator* bo )
{
    if ( body0 )
    {
	if ( inputbodyop0_ ) delete inputbodyop0_;
	inputbodyop0_ = bo;
	inputbody0_ = MultiID(0);
    }
    else
    {
	if ( inputbodyop1_ ) delete inputbodyop1_;
	inputbodyop1_ = bo;
	inputbody1_ = MultiID(0);
    }
}


void BodyOperator::setAction( Action act )
{ action_ = act; }


BodyOperator* BodyOperator::getChildOprt( bool body0 ) const
{ return body0 ? inputbodyop0_ : inputbodyop1_; }


bool BodyOperator::getChildOprt( int freeid, BodyOperator& res )
{
   if ( freeid==id_ )
   {
       res = *this;
       return true;
   }

   for ( int idx=0; idx<2; idx++ )
   {
       if ( getChildOprt(!idx) )
	   return getChildOprt(!idx)->getChildOprt( freeid, res );
   }

    return false; 
}


ImplicitBody* BodyOperator::getOperandBody( bool body0, TaskRunner* tr ) const
{
    ImplicitBody* body = 0;
    BodyOperator* oprt = body0 ? inputbodyop0_ : inputbodyop1_;
    if ( !oprt )
    {
	const MultiID mid = body0 ? inputbody0_ : inputbody1_;
	const char* translt = IOM().get( mid )->translator();
	EM::EMObject* obj = 0;
	if ( !strcmp( translt, polygonEMBodyTranslator::sKeyUserName() ) )
	{
	    obj = EMM().createTempObject(EM::PolygonBody::typeStr());
	}
	else if ( !strcmp( translt, randposEMBodyTranslator::sKeyUserName() ) )
	{
	    obj = EMM().createTempObject(EM::RandomPosBody::typeStr());
	}
	else if ( !strcmp( translt, mcEMBodyTranslator::sKeyUserName() ) )
	{
	    obj = EMM().createTempObject(EM::MarchingCubesSurface::typeStr());
	}

	if ( !obj ) return 0;
	obj->ref();
	
	obj->setMultiID( mid );
	if ( !obj->loader() || !obj->loader()->execute() )
	{
	    obj->unRef();
	    return 0;
	}
	
	mDynamicCastGet( Body*, embody, obj );
	if ( !embody ) 
	{
	    obj->unRef();
	    return 0;
	}

	body = embody->createImplicitBody( tr, false );
	obj->unRef();
    }
    else if ( !oprt->createImplicitBody( body, tr ) )
    {
	delete body;
	body = 0;
    }
	
    return body;
}

bool BodyOperator::createImplicitBody( ImplicitBody*& res, TaskRunner* tr) const
{
    if ( !isOK() ) return false;

    ImplicitBody* b0 = getOperandBody( true, tr );
    if ( !b0 ) return false;

    ImplicitBody* b1 = getOperandBody( false, tr );
    if ( !b1 ) 
    {
	delete b0;
	return false;
    }

    //Case 1: at least one implicit body is 0.
    if ( !b0 || !b1 )
    {
	if ( action_==IntSect )
	    res = 0;
	else if ( action_==Union )
	    res = b0 ? b0 : b1;
	else
	    res = !b1 ? b0 : 0;

	return true;
    }

    //Case 2: two bodies exist but there is no array for at least one of them.
    if ( !b0->arr_ || !b1->arr_ )
    {
	res = 0;
	return true;//May discuss more similar to case 1.
    }

    //Set new body ranges.
    const SamplingData<int>& inlsmp0 = b0->inlsampling_;
    const int inlstop0 = 
	inlsmp0.start+inlsmp0.step*(b0->arr_->info().getSize(0)-1);

    const SamplingData<int>& inlsmp1 = b1->inlsampling_;
    const int inlstop1 = 
	inlsmp1.start+inlsmp1.step*(b1->arr_->info().getSize(0)-1);

    const SamplingData<int>& crlsmp0 = b0->crlsampling_;
    const int crlstop0 = 
	crlsmp0.start+crlsmp0.step*(b0->arr_->info().getSize(1)-1);

    const SamplingData<int>& crlsmp1 = b1->crlsampling_;
    const int crlstop1 = 
	crlsmp1.start+crlsmp1.step*(b1->arr_->info().getSize(1)-1);
    
    const SamplingData<float>& zsmp0 = b0->zsampling_;
    const float zstop0 = 
	zsmp0.start+zsmp0.step*(b0->arr_->info().getSize(2)-1);

    const SamplingData<float>& zsmp1 = b1->zsampling_;
    const float zstop1 =
	zsmp1.start+zsmp1.step*(b1->arr_->info().getSize(2)-1);

    //If action is Minus, make the new cube the same size as body0.
    StepInterval<int> newinlrg( inlsmp0.start, inlstop0, 
	    			mMIN( inlsmp0.step, inlsmp1.step ) );
    StepInterval<int> newcrlrg( crlsmp0.start, crlstop0, 
	    			mMIN( crlsmp0.step, crlsmp1.step ) );
    StepInterval<float> newzrg( zsmp0.start, zstop0,
	    			mMIN( zsmp0.step, zsmp1.step ) );
    if ( action_==Union )
    {
	newinlrg.start = mMIN( inlsmp0.start, inlsmp1.start );
	newinlrg.stop = mMAX( inlstop0, inlstop1 );
	
	newcrlrg.start = mMIN( crlsmp0.start, crlsmp1.start );
	newcrlrg.stop = mMAX( crlstop0, crlstop1 );
	
	newzrg.start = mMIN( zsmp0.start, zsmp1.start );
	newzrg.stop = mMAX( zstop0, zstop1 );
    }
    else if ( action_==IntSect )
    {
	newinlrg.start = mMAX( inlsmp0.start, inlsmp1.start );
	newinlrg.stop = mMIN( inlstop0, inlstop1 );
	
	newcrlrg.start = mMAX( crlsmp0.start, crlsmp1.start );
	newcrlrg.stop = mMIN( crlstop0, crlstop1 );
	
	newzrg.start = mMAX( zsmp0.start, zsmp1.start );
	newzrg.stop = mMIN( zstop0, zstop1 );
    }

    if ( newinlrg.start>newinlrg.stop || newcrlrg.start>newcrlrg.stop || 
	 newzrg.start>newzrg.stop )
    {
	res = 0;
	return true;
    }

    //Add one layer to the cube to make MarchingCubes display boundary nicely.
    if ( newinlrg.start-newinlrg.step>=0 ) newinlrg.start -= newinlrg.step;
    if ( newcrlrg.start-newcrlrg.step>=0 ) newcrlrg.start -= newcrlrg.step;
    if ( newzrg.start-newzrg.step>=0 ) newzrg.start -= newzrg.step;

    newinlrg.stop += newinlrg.step;
    newcrlrg.stop += newcrlrg.step;
    newzrg.stop += newzrg.step;

    mTryAlloc(res,ImplicitBody);
    if ( !res )
    {
	delete b0;
	delete b1;
	return false;
    }

    mDeclareAndTryAlloc(Array3D<float>*,arr,Array3DImpl<float>(
		newinlrg.nrSteps()+1,newcrlrg.nrSteps()+1,newzrg.nrSteps()+1));
    if ( !arr )
    {
	res = 0;
	delete b0;
	delete b1;
	return false;
    }
   
    BodyOperatorArrayFiller arrfiller( *b0, *b1, newinlrg, newcrlrg, newzrg,
	    				action_, *arr );
    arrfiller.execute();

    res->arr_ = arr;
    res->threshold_ = 0;
    res->inlsampling_.start = newinlrg.start;
    res->inlsampling_.step  = newinlrg.step;
    res->crlsampling_.start = newcrlrg.start;
    res->crlsampling_.step  = newcrlrg.step;
    res->zsampling_.start   = newzrg.start;
    res->zsampling_.step    = newzrg.step;

    return true;    
}


ImplicitBody* BodyOperator::createImplicitBody( const TypeSet<Coord3>& bodypts,
						TaskRunner* tr ) const
{
    if ( bodypts.size()<3 )
	return 0;
    
    StepInterval<int> inlrg( 0, 0, SI().inlStep() );
    StepInterval<int> crlrg( 0, 0, SI().crlStep() );
    StepInterval<float> zrg( 0, 0, SI().zStep() );
    
    for ( int idx=0; idx<bodypts.size(); idx++ )
    {
	const BinID bid = SI().transform( bodypts[idx].coord() );
	
	if ( !idx )
	{
	    inlrg.start = inlrg.stop = bid.inl;
	    crlrg.start = crlrg.stop = bid.crl;
	    zrg.start = zrg.stop = bodypts[idx].z;
	}
	else
	{
	    inlrg.include( bid.inl );
	    crlrg.include( bid.crl );
	    zrg.include( bodypts[idx].z );
	}
    }
    
    if ( inlrg.start-inlrg.step>=0 ) inlrg.start -= inlrg.step;
    if ( crlrg.start-crlrg.step>=0 ) crlrg.start -= crlrg.step;
    if ( zrg.start-zrg.step>=0 ) zrg.start -= zrg.step;
    if ( inlrg.stop+inlrg.step<=SI().inlRange(true).stop )
    	inlrg.stop += inlrg.step;

    if ( crlrg.stop+crlrg.step<=SI().crlRange(true).stop )
	crlrg.stop += crlrg.step;

    if ( zrg.stop+zrg.step<=SI().zRange(true).stop )
    	zrg.stop += zrg.step;
    
    mDeclareAndTryAlloc( Array3D<float>*, arr, Array3DImpl<float>( 
		inlrg.nrSteps()+1, crlrg.nrSteps()+1, zrg.nrSteps()+1 ) );
    if ( !arr ) return 0; 

    mDeclareAndTryAlloc(ImplicitBody*,res,ImplicitBody);
    if ( !res )
    {
	delete arr;
	return 0;
    }
    
    MouseCursorChanger cursorchanger(MouseCursor::Wait);
    
    TypeSet<Coord3> pts = bodypts;
    const float zscale = SI().zFactor();
    if ( !mIsZero(zscale-1,1e-5) )
    {
    	for ( int idx=0; idx<bodypts.size(); idx++ )
    	    pts[idx].z *= zscale;
    }

    DAGTetrahedraTree dagtree;
    if ( !dagtree.setCoordList( pts, false ) )
	return 0;
    
    ParallelDTetrahedralator triangulator( dagtree );
    if ( (tr && !tr->execute( triangulator )) || !triangulator.execute(true) )
	return 0;
   
    PtrMan<Expl2ImplBodyExtracter> extract = new
	Expl2ImplBodyExtracter( dagtree, inlrg, crlrg, zrg, *arr );
    if ( (tr && !tr->execute( *extract )) || !extract->execute() )
	return 0;

    res->arr_ = arr;
    res->threshold_ = 0;
    res->inlsampling_.start = inlrg.start;
    res->inlsampling_.step  = inlrg.step;
    res->crlsampling_.start = crlrg.start;
    res->crlsampling_.step  = crlrg.step;
    res->zsampling_.start   = zrg.start;
    res->zsampling_.step    = zrg.step;
    
    cursorchanger.restore();
    return res;
}


bool BodyOperator::usePar( const IOPar& par )
{
    
    par.get( sKeyBodyID0(), inputbody0_ );
    par.get( sKeyBodyID1(), inputbody1_ );
    
    if( (inputbodyop0_ && !inputbodyop0_->usePar(par)) ||
   	(inputbodyop1_ && !inputbodyop1_->usePar(par)) )
	return false;

    if ( !par.get( sKeyID(), id_ ) )
	return false;

    int act;
    if ( !par.get( sKeyAction(), act ) )
	return false;

    action_ = act==0 ? Union : (act==1 ? IntSect : Minus);

    return true;
}


void BodyOperator::fillPar( IOPar& par )
{
    if ( inputbodyop0_ )  inputbodyop0_->fillPar(par);
    if ( inputbodyop1_ )  inputbodyop1_->fillPar(par);
    
    par.set( sKeyBodyID0(), inputbody0_ );
    par.set( sKeyBodyID1(), inputbody1_ );
    par.set( sKeyID(), id_ );
    
    const int act = action_==Union ? 0 : (action_==IntSect ? 1 : 2);
    par.set( sKeyAction(), act );
}


}; //Namespce
