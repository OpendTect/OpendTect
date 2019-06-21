/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Yuancheng Liu
 Date:          Feb 2009
________________________________________________________________________

-*/

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
#include "survinfo.h"
#include "task.h"
#include "ioobj.h"
#include "executor.h"


namespace EM
{

#define mInsideVal	1
#define mOutsideVal	-1
#define mOnBodyVal	0

class BodyOperatorArrayFiller: public ParallelTask
{ mODTextTranslationClass(BodyOperatorArrayFiller);
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

float	 getThreshold() const	{ return 0.f; }
od_int64 nrIterations() const	{ return arr_.totalSize(); }
uiString message() const { return tr("Calculating implicit body operation"); }

protected:

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    for ( int idx=mCast(int,start); idx<=stop && shouldContinue(); idx++ )
    {
	int p[3];
	arr_.info().getArrayPos( idx, p );

	const int inl = inlrg_.atIndex(p[0]);
	const int crl = crlrg_.atIndex(p[1]);
	const float z = zrg_.atIndex(p[2]);

	int id0[3], id1[3];
	id0[0] = b0_.tkzs_.inlIdx( inl );
	id0[1] = b0_.tkzs_.crlIdx( crl );
	id0[2] = b0_.tkzs_.zIdx( z );
	id1[0] = b1_.tkzs_.inlIdx( inl );
	id1[1] = b1_.tkzs_.crlIdx( crl );
	id1[2] = b1_.tkzs_.zIdx( z );

	char pos0 = mOutsideVal, pos1 = mOutsideVal;
	float v0 = b0_.threshold_+mOutsideVal, v1 = b1_.threshold_+mOutsideVal;
	if ( b0_.arr_->info().validPos(id0[0],id0[1],id0[2]) )
	{
	    v0 = b0_.arr_->get( id0[0], id0[1], id0[2] );
	    pos0 = (v0>b0_.threshold_) ? mInsideVal
		 : (v0<b0_.threshold_ ? mOutsideVal : mOnBodyVal);
	}

	if ( b1_.arr_->info().validPos(id1[0],id1[1],id1[2]) )
	{
	    v1 = b1_.arr_->get( id1[0], id1[1], id1[2] );
	    pos1 = (v1>b1_.threshold_) ? mInsideVal
		 : (v1<b1_.threshold_ ? mOutsideVal : mOnBodyVal);
	}

	const float val = getVal( pos0, pos1, v0, v1 );
	arr_.set( p[0], p[1], p[2], val );
	addToNrDone( 1 );
    }

    return true;
}


float getVal( char p0, char p1, float v0, float v1 ) const
{
    float res = mOutsideVal;
    const bool useval = mIsEqual(b0_.threshold_,b1_.threshold_,1e-5);

    if ( p0==mInsideVal )
    {
	if ( p1==mInsideVal )
	{
	    if ( action_==BodyOperator::Minus )
	    {
		res = useval ? b1_.threshold_-v1 : mOutsideVal;
	    }
	    else
	    {
		res = useval ? mMAX(v0,v1)-b1_.threshold_ : mInsideVal;
	    }
	}
	else if ( p1==mOnBodyVal )
	{
	    if ( action_==BodyOperator::Union )
	    {
		res = useval ? v0-b0_.threshold_ : mInsideVal;
	    }
	    else if ( action_==BodyOperator::IntSect )
	    {
		res = mOnBodyVal;;
	    }
	    else
	    {
		res = useval ? -0.01f : mOutsideVal;
	    }
	}
	else
	{
	    if ( action_==BodyOperator::IntSect )
	    {
		res = useval ? v1-b1_.threshold_ : mOutsideVal;
	    }
	    else
	    {
		res = useval ? v0-b0_.threshold_ : mInsideVal;
	    }
	}
    }
    else if ( p0==mOnBodyVal )
    {
	if ( p1==mInsideVal )
	{
	    if ( action_==BodyOperator::Union )
	    {
		res = useval ? v1-b1_.threshold_ : mInsideVal;
	    }
	    else if ( action_==BodyOperator::IntSect )
	    {
		res = mOnBodyVal;
	    }
	    else
	    {
		res = useval ? b1_.threshold_-v1 : mOutsideVal;
	    }
	}
	else if ( p1==mOnBodyVal )
	{
	    if ( action_==BodyOperator::Minus )
		res = useval ? -0.01f : mOutsideVal;
	    else
		res = mOnBodyVal;
	}
	else
	{
	    if ( action_==BodyOperator::IntSect )
	    {
		res = useval ? -0.01f : mOutsideVal;
	    }
	    else
	    {
		res = mOnBodyVal;
	    }
	}
    }
    else
    {
	if ( p1==mInsideVal )
	{
	    if ( action_==BodyOperator::Union )
	    {
		res = useval ? v1-b1_.threshold_ : mInsideVal;
	    }
	    else
	    {
		res = useval ? v0-b0_.threshold_ : mOutsideVal;
	    }
	}
	else if ( p1==mOnBodyVal )
	{
	    if ( action_==BodyOperator::Union )
	    {
		res = mOnBodyVal;
	    }
	    else
	    {
		res = useval ? v0-b0_.threshold_ : mOutsideVal;
	    }
	}
	else
	{
	    res = useval ? v0-b0_.threshold_ : mOutsideVal;
	}
    }

    return res;
}

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
{}


bool Expl2ImplBodyExtracter::doPrepare( int nrthreads )
{
    planes_.erase();

    const TypeSet<Coord3>& crds = tree_.coordList();
    tree_.getSurfaceTriangles( tri_ );

    const int planesz = tri_.size()/3;

    for ( int idx=0; idx<planesz; idx++ )
    {
	const int startid = 3 * idx;
	Coord3 nm = (crds[tri_[startid]]- crds[tri_[startid+1]]).cross(
		crds[tri_[startid+1]]-crds[tri_[startid+2]]);
	planes_ += Plane3(nm.normalize(),crds[tri_[startid]],false);
    }

    return planesz;
}


od_int64 Expl2ImplBodyExtracter::nrIterations() const
{ return arr_.getSize(0)*arr_.getSize(1); }


#define mSetSegment() \
if ( !nrintersections ) \
    segment.start = segment.stop = (float) pos.z_; \
else \
    segment.include( (float) pos.z_ ); \
nrintersections++


bool Expl2ImplBodyExtracter::doWork( od_int64 start, od_int64 stop, int )
{
    const TypeSet<Coord3>& crds = tree_.coordList();
    const int crlsz = arr_.getSize(1);
    const int zsz = arr_.getSize(2);
    const int planesize = planes_.size();

    for ( int idx=mCast(int,start); idx<=stop && shouldContinue(); idx++ )
    {
	const int inlidx = idx / crlsz;
	const int crlidx = idx % crlsz;
	const BinID bid( inlrg_.atIndex(inlidx), crlrg_.atIndex(crlidx) );
	Coord3 pos( SI().transform(bid), 0 );
	Line3 vtln = Line3::fromPosAndDir( pos, Coord3(0,0,1) );

	Interval<float> segment;
	int nrintersections = 0;
	for ( int pl=0; pl<planesize; pl++ )
	{
	    Coord3 v[3];
	    for ( int pidx=0; pidx<3; pidx++ )
		v[pidx] = crds[tri_[3*pl+pidx]];

	    const double fv = planes_[pl].A_*pos.x_ + planes_[pl].B_*pos.y_ +
		planes_[pl].D_;
	    if ( mIsZero(planes_[pl].C_,1e-3) )
	    {
		if ( mIsZero(fv,1e-3) )
		{
		    for ( int pidx=0; pidx<3; pidx++ )
		    {
			const Coord3 dir = v[(pidx+1)%3]-v[pidx];
			if ( mIsZero(dir.x_,1e-3) && mIsZero(dir.y_,1e-3) )
			{
			    const Coord diff = pos.getXY() - v[pidx].getXY();
			    if ( mIsZero(diff.x_,1e-3)&& mIsZero(diff.y_,1e-3) )
			    {
				pos.z_ = v[pidx].z_;
				mSetSegment();
				pos.z_ = v[(pidx+1)%3].z_;
				mSetSegment();
			    }
			}
			else
			{
			    Line3 edge = Line3::fromPosAndDir( v[pidx], dir );
			    double te, tv;
			    vtln.closestPointToLine(edge,tv,te);
			    if ( te<=1 && te>=0 )
			    {
				pos.z_ = v[pidx].z_+te*dir.z_;
				mSetSegment();
			    }
			}
		    }
		}
	    }
	    else
	    {
		pos.z_ = -fv/planes_[pl].C_;
		if ( pointInTriangle3D(pos,v[0],v[1],v[2],0) )
		{
		    mSetSegment();
		}
		else
		{
		    if ( pointOnEdge3D( pos, v[0], v[1], 1e-3 ) ||
			 pointOnEdge3D( pos, v[1], v[2], 1e-3 ) ||
			 pointOnEdge3D( pos, v[2], v[0], 1e-3 ) )
		    {
			mSetSegment();
		    }
		}
	    }
	}

	if ( nrintersections )
	{
	    for ( int zidx=0; zidx<zsz; zidx++ )
	    {
		const float curz = zrg_.atIndex( zidx );
		const float val = curz<segment.start ? curz-segment.start :
		    ( curz>segment.stop ? segment.stop-curz :
		      (nrintersections>2 ? 0.f :
		      mMIN(curz-segment.start, segment.stop-curz)) );
		arr_.set( inlidx, crlidx, zidx, val );
	    }
	}
	else
	{
	    for ( int zidx=0; zidx<zsz; zidx++ )
		arr_.set( inlidx, crlidx, zidx, mOutsideVal );
	}

	addToNrDone( 1 );
    }

    return true;
}


/*
//Go by each point: too slow
    //Move the folowing to doPrepare
    Coord3 bodycenter(0,0,0);
    TypeSet<int> vertices;

#define mSetVetex( idy ) \
    if ( vertices.indexOf(tri_[idy])==-1 ) \
    { \
	vertices += tri_[idy]; \
	bodycenter += crds[tri_[idy]]; \
    }
	mSetVetex( startid );
	mSetVetex( startid+1 );
	mSetVetex( startid+2 );
    }

    const int vsz = vertices.size();
    if ( vsz<3 ) return false;
    bodycenter /= vsz;
    return true; //to doPrepare

bool Expl2ImplBodyExtracter::doP2P( od_int64 start, od_int64 stop )
{
    const TypeSet<Coord3>& crds = tree_.coordList();
    const int planesize = planes_.size();
    const int crlsz = arr_.getSize(1);
    const int zsz = arr_.getSize(2);

    for ( int idx=start; idx<=stop && shouldContinue(); idx++, addToNrDone(1) )
    {
	const int inlidx = idx / crlsz;
	const int crlidx = idx % crlsz;
	const BinID bid( inlrg_.atIndex(inlidx), crlrg_.atIndex(crlidx) );
	Coord3 pos( SI().transform(bid), 0 );

	for ( int zidx=0; zidx<zsz; zidx++ )
	{
	    pos.z_ = zrg_.atIndex( zidx );

	    char val = -1;
	    for ( int pl=0; pl<planesize; pl++ )
	    {
		if ( val!=-1 )
		    break;

		const float pv = planes_[pl].A_*pos.x_ + planes_[pl].B_*pos.y_ +
		    planes_[pl].C_*pos.z_ + planes_[pl].D_;

		const float bpv= planes_[pl].A_*bodycenter.x_ +
		    planes_[pl].B_*bodycenter.y_ +
		    planes_[pl].C_*bodycenter.z_ + planes_[pl].D_;

	       if ( pv*bpv>0 || mIsZero(bpv,1e-3) )
		   continue;
	       else if ( pv*bpv<0 )
		   val = 1;
	       else
	       {
		   for ( int pi=0; pi<planesize; pi++ )
		   {
		       if ( pi==pl )
			   continue;

		       float piv = planes_[pi].A_*pos.x_ + planes_[pi].B_*pos.y_
				   + planes_[pi].C_*pos.z_ + planes_[pi].D_;
		       if ( !mIsZero(piv,1e-3) )
			   continue;

		       Coord3 v[3];
		       for ( int pidx=0; pidx<3; pidx++ )
			   v[pidx] = crds[tri_[3*pi+pidx]];

		       if ( pointInTriangle3D(pos,v[0],v[1],v[2],0) ||
			       pointOnEdge3D( pos, v[0], v[1], 1e-3 ) ||
			       pointOnEdge3D( pos, v[1], v[2], 1e-3 ) ||
			       pointOnEdge3D( pos, v[2], v[0], 1e-3 ) )
		       {
			   val = 0;
			   break;
		       }
		   }

		   if ( val!=0 )
		       val = 1;
		}
	    }

	    arr_.set( inlidx, crlidx, zidx, val );
	}
    }

    return true;
}*/


BodyOperator::BodyOperator()
    : inputbodyop0_( 0 )
    , inputbodyop1_( 0 )
    , inputbody0_( DBKey::getInvalid() )
    , inputbody1_( DBKey::getInvalid() )
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
    mDefineStaticLocalObject( Threads::Atomic<int>, id, (0));
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
	if ( !inputbody0_.isUsable() )
	    return false;
    }

    if ( inputbodyop1_ )
    {
	if ( !inputbodyop1_->isOK() )
	    return false;
    }
    else
    {
	if ( !inputbody1_.isUsable() )
	    return false;
    }

    return true;
}


void BodyOperator::setInput( bool body0, const DBKey& mid )
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
	inputbody0_ = DBKey::getInvalid();
    }
    else
    {
	if ( inputbodyop1_ ) delete inputbodyop1_;
	inputbodyop1_ = bo;
	inputbody1_ = DBKey::getInvalid();
    }
}


void BodyOperator::setAction( Action act )
{ action_ = act; }


BodyOperator* BodyOperator::getChildOprt( bool body0 ) const
{ return body0 ? inputbodyop0_ : inputbodyop1_; }


bool BodyOperator::getChildOprt( int freeid, BodyOperator& res )
{
    if ( freeid==id_ )
	{ res = *this; return true; }

    for ( bool forbody0 : {true,false} )
    {
	auto* inpoper = getChildOprt( forbody0 );
	if ( inpoper )
	    return inpoper->getChildOprt( freeid, res );
    }

    return false;
}


ImplicitBody* BodyOperator::getOperandBody( bool body0,
				const TaskRunnerProvider& trprov ) const
{
    ImplicitBody* body = 0;
    BodyOperator* oprt = body0 ? inputbodyop0_ : inputbodyop1_;
    if ( !oprt )
    {
	const DBKey mid = body0 ? inputbody0_ : inputbody1_;
	const BufferString translt = BodyMan().objectType(mid);
	EM::Object* obj = BodyMan().createTempObject( translt );

	if ( !obj ) return 0;
	obj->ref();

	obj->setDBKey( mid );
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

	body = embody->createImplicitBody( trprov, false );
	obj->unRef();
    }
    else if ( !oprt->createImplicitBody( body, trprov ) )
    {
	delete body;
	body = 0;
    }

    return body;
}

bool BodyOperator::createImplicitBody( ImplicitBody*& res,
					const TaskRunnerProvider& trprov ) const
{
    if ( !isOK() ) return false;

    ImplicitBody* b0 = getOperandBody( true, trprov );
    if ( !b0 ) return false;

    ImplicitBody* b1 = getOperandBody( false, trprov );
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
	    res = b0 ? b0 : 0;

	return true;
    }

    //Case 2: two bodies exist but there is no array for at least one of them.
    if ( !b0->arr_ || !b1->arr_ )
    {
	res = 0;
	return true;//May discuss more similar to case 1.
    }

    //Set new body ranges.
    const StepInterval<int>& inlrg0 = b0->tkzs_.hsamp_.inlRange();
    const StepInterval<int>& crlrg0 = b0->tkzs_.hsamp_.crlRange();
    const StepInterval<float>& zrg0 = b0->tkzs_.zsamp_;
    const StepInterval<int>& inlrg1 = b1->tkzs_.hsamp_.inlRange();
    const StepInterval<int>& crlrg1 = b1->tkzs_.hsamp_.crlRange();
    const StepInterval<float>& zrg1 = b1->tkzs_.zsamp_;

    //If action is Minus, make the new cube the same size as body0.
    StepInterval<int> newinlrg( inlrg0.start, inlrg0.stop,
				mMIN( inlrg0.step, inlrg1.step ) );
    StepInterval<int> newcrlrg( crlrg0.start, crlrg0.stop,
				mMIN( crlrg0.step, crlrg1.step ) );
    StepInterval<float> newzrg(zrg0.start,zrg0.stop,mMIN(zrg0.step,zrg1.step));
    if ( action_==Union )
    {
	newinlrg.start = mMIN( inlrg0.start, inlrg1.start );
	newinlrg.stop = mMAX( inlrg0.stop, inlrg1.stop );

	newcrlrg.start = mMIN( crlrg0.start, crlrg1.start );
	newcrlrg.stop = mMAX( crlrg0.stop, crlrg1.stop );

	newzrg.start = mMIN( zrg0.start, zrg1.start );
	newzrg.stop = mMAX( zrg0.stop, zrg1.stop );
    }
    else if ( action_==IntSect )
    {
	newinlrg.start = mMAX( inlrg0.start, inlrg1.start );
	newinlrg.stop = mMIN( inlrg0.stop, inlrg1.stop );

	newcrlrg.start = mMAX( crlrg0.start, crlrg1.start );
	newcrlrg.stop = mMIN( crlrg0.stop, crlrg1.stop );

	newzrg.start = mMAX( zrg0.start, zrg1.start );
	newzrg.stop = mMIN( zrg0.stop, zrg1.stop );
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
    res->threshold_ = arrfiller.getThreshold();
    res->tkzs_.hsamp_.start_ = BinID(newinlrg.start, newcrlrg.start);
    res->tkzs_.hsamp_.stop_ = BinID(newinlrg.stop, newcrlrg.stop);
    res->tkzs_.hsamp_.step_ = BinID(newinlrg.step, newcrlrg.step);
    res->tkzs_.zsamp_ = newzrg;

    return true;
}


ImplicitBody* BodyOperator::createImplicitBody( const TypeSet<Coord3>& bodypts,
					const TaskRunnerProvider& trprov ) const
{
    if ( bodypts.size()<3 )
	return 0;

    StepInterval<int> inlrg( 0, 0, SI().inlStep() );
    StepInterval<int> crlrg( 0, 0, SI().crlStep() );
    StepInterval<float> zrg( 0, 0, SI().zStep() );

    for ( int idx=0; idx<bodypts.size(); idx++ )
    {
	const BinID bid = SI().transform( bodypts[idx].getXY() );

	if ( !idx )
	{
	    inlrg.start = inlrg.stop = bid.inl();
	    crlrg.start = crlrg.stop = bid.crl();
	    zrg.start = zrg.stop = (float) bodypts[idx].z_;
	}
	else
	{
	    inlrg.include( bid.inl() );
	    crlrg.include( bid.crl() );
	    zrg.include( (float) bodypts[idx].z_ );
	}
    }

    if ( inlrg.start-inlrg.step>=0 ) inlrg.start -= inlrg.step;
    if ( crlrg.start-crlrg.step>=0 ) crlrg.start -= crlrg.step;
    if ( zrg.start-zrg.step>=0 ) zrg.start -= zrg.step;
    if ( inlrg.stop+inlrg.step<=SI().inlRange().stop )
	inlrg.stop += inlrg.step;

    if ( crlrg.stop+crlrg.step<=SI().crlRange().stop )
	crlrg.stop += crlrg.step;

    if ( zrg.stop+zrg.step<=SI().zRange().stop )
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

    TypeSet<Coord3> pts = bodypts;
    const int zscale = SI().zDomain().userFactor();
    if ( zscale != 1 )
    {
	for ( int idx=0; idx<bodypts.size(); idx++ )
	    pts[idx].z_ *= zscale;
    }

    DAGTetrahedraTree dagtree;
    if ( !dagtree.setCoordList( pts, false ) )
	return 0;

    ParallelDTetrahedralator triangulator( dagtree );
    if ( trprov.execute( triangulator ) )
    {
	StepInterval<float> tmpzrg( zrg ); tmpzrg.scale( mCast(float,zscale) );
	PtrMan<Expl2ImplBodyExtracter> extract = new
	    Expl2ImplBodyExtracter( dagtree, inlrg, crlrg, tmpzrg, *arr );
	if ( trprov.execute( *extract ) )
	{
	    res->arr_ = arr;
	    res->threshold_ = 0;
	    res->tkzs_.hsamp_.start_ = BinID(inlrg.start, crlrg.start);
	    res->tkzs_.hsamp_.stop_ = BinID(inlrg.stop, crlrg.stop);
	    res->tkzs_.hsamp_.step_ = BinID(inlrg.step, crlrg.step);
	    res->tkzs_.zsamp_ = zrg;
	    return res;
	}
    }

    return 0;
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
