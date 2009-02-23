/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Yuancheng Liu
 Date:          Feb 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: embodyoperator.cc,v 1.2 2009-02-23 04:39:28 cvsnanne Exp $";

#include "embodyoperator.h"

#include "arrayndimpl.h"
#include "delaunay3d.h"
#include "embody.h"
#include "mousecursor.h"
#include "survinfo.h"
#include "task.h"


namespace EM
{

mClass BodyOperatorArrayFiller: public ParallelTask
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

od_int64 totalNr() const { return arr_.info().getTotalSz(); }
		       
protected:

bool doWork( od_int64 start, od_int64 stop, int threadid )    
{
    for ( int idx=start; idx<=stop && shouldContinue(); idx++ )
    {
	int p[3];
	arr_.info().getArrayPos( idx, p );
	
	const int idx0 = b0_.inlsampling_.nearestIndex( inlrg_.atIndex(p[0]) );
	const int idx1 = b1_.inlsampling_.nearestIndex( inlrg_.atIndex(p[0]) );
	const int idy0 = b0_.crlsampling_.nearestIndex( crlrg_.atIndex(p[1]) );
	const int idy1 = b1_.crlsampling_.nearestIndex( crlrg_.atIndex(p[1]) );
	const int idz0 = b0_.zsampling_.nearestIndex( zrg_.atIndex(p[2]) );
	const int idz1 = b1_.zsampling_.nearestIndex( zrg_.atIndex(p[2]) );
	const bool incube0 = b0_.arr_->info().validPos( idx0, idy0, idz0 );
	const bool incube1 = b1_.arr_->info().validPos( idx1, idy1, idz1 );

	char pos0 = cOutsde(), pos1 = cOutsde();
	if ( incube0 )
	{
	    const float v0 = b0_.arr_->get(idx0,idy0,idz0);
	    pos0 = (v0<b0_.threshold_) ? cInside() 
		 : (v0>b0_.threshold_ ? cOutsde() : cOnbody());
	}

	if ( incube1 )
	{
	    const float v1 = b1_.arr_->get(idx1,idy1,idz1);
	    pos1 = (v1<b1_.threshold_) ? cInside() 
		 : (v1>b1_.threshold_ ? cOutsde() : cOnbody());
	}
	
	arr_.set( p[0], p[1], p[2], getVal(incube0,incube1,pos0,pos1) );
    }

    return true;
}


char getVal( bool incube0, bool incube1, char p0, char p1 ) const
{
    if ( !incube0 || !incube1 )
    {
	if ( incube0 )
	    return action_==BodyOperator::IntSect ? cOutsde() : p0;
	else if ( incube1 )
	    return action_==BodyOperator::Union ? p1 : cOutsde();
	else
	    return cOutsde();
    }

    if ( action_==BodyOperator::Union )
    {
	return (p0==cInside() || p1==cInside()) ? cInside() : 
	    ( (p0==cOnbody() || p1==cOnbody()) ? cOnbody() : cOutsde() );
    }
    else if ( action_==BodyOperator::IntSect )
    {
	return (p0==cInside() && p1==cInside()) ? cInside() : 
	    ( ((p0==cOnbody() && p1==cInside()) || 
	       (p0==cInside() && p1==cOnbody()) || 
	       (p0==cOnbody() && p1==cOnbody())) ? cOnbody() : cOutsde() );
    }
    else 
    {
	return (p0==cOutsde() || p1==cInside()) ? cOutsde() : 
	    ( (p0==cInside() && p1==cOutsde()) ? cInside() : cOnbody() );
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
    const StepInterval<float>&	zrg_;
    BodyOperator::Action	action_;
};


BodyOperator::BodyOperator()
    : inputbody0_( 0 )
    , inputbody1_( 0 )
    , inputbodyop0_( 0 )
    , inputbodyop1_( 0 )
    , id_( 0 )
    , childid_( -1 )	       
    , action_( Union )			       
{}


#define mRemoveBodyImpl( bodynr ) \
	if ( inputbodyop##bodynr##_ ) \
	{ \
	    delete inputbodyop##bodynr##_; \
	    inputbodyop##bodynr##_ = 0; \
	} \
	if ( inputbody##bodynr##_ ) \
	{ \
	    inputbody##bodynr##_->unRefBody(); \
	    inputbody##bodynr##_ = 0; \
	}

BodyOperator::~BodyOperator()
{
    mRemoveBodyImpl( 0 );
    mRemoveBodyImpl( 1 );
}


bool BodyOperator::isOK() const
{
    return (inputbody0_ || (inputbodyop0_&&inputbodyop0_->isOK()) ) && 
	   (inputbody1_ || (inputbodyop1_&&inputbodyop1_->isOK()) );
}


void BodyOperator::setInput( bool body0, Body* bd )
{
    if ( body0 )
    {
	mRemoveBodyImpl( 0 );
	inputbody0_ = bd;
	if ( inputbody0_ ) inputbody0_->refBody();
    }
    else
    {
	mRemoveBodyImpl( 1 );
	inputbody1_ = bd;
	if ( inputbody1_ ) inputbody1_->refBody();
    }
}


void BodyOperator::setInput( bool body0, BodyOperator* bo )
{
    if ( body0 )
    {
	mRemoveBodyImpl( 0 );
	inputbodyop0_ = bo;
    }
    else
    {
	mRemoveBodyImpl( 1 );
	inputbodyop1_ = bo;
    }

    if ( bo )
    {
	id_ = bo->getID();
	childid_ = id_+1;
    }
}


void BodyOperator::setAction( Action act )
{ action_ = act; }


void BodyOperator::setParent( BodyOperator* parent )
{
    if ( !parent )
	return;

    id_ = parent->getChildID();
    childid_ = id_+1;
}


bool BodyOperator::createImplicitBody( ImplicitBody*& res, TaskRunner* tr) const
{
    if ( !isOK() ) return false;

    //Create two implicit bodies.
    ImplicitBody* b0;
    if ( inputbody0_ ) 
	b0 = inputbody0_->createImplicitBody( tr );
    else if ( !inputbodyop0_->createImplicitBody( b0, tr ) )
	return false;

    ImplicitBody* b1;
    if ( inputbody1_ ) 
	b1 = inputbody1_->createImplicitBody( tr );
    else if ( !inputbodyop1_->createImplicitBody( b1, tr ) )
    {
	delete b0;
	return false;
    }

    //Case 1: at least one implicit body is 0.
    if ( !b0 || !b1 )
    {
	if ( action_==IntSect )
	{
	    delete b0;
	    delete b1;
	    res = 0;
	}
	else if ( action_==Union )
	{
	    res = b0 ? b0 : b1;
	}
	else
	{
	    if ( !b1 )
		res = b0;
	    else
	    {
		delete b1;
		res = 0;
	    }
	}

	return true;
    }

    //Case 2: two bodies exist but there is no array for at least one of them.
    if ( !b0->arr_ || !b1->arr_ )
    {
	delete b0;
	delete b1;
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
    if ( newinlrg.start-newinlrg.step>=0 )
	newinlrg.start -= newinlrg.step;
    if ( newcrlrg.start-newcrlrg.step>=0 )
	newcrlrg.start -= newcrlrg.step;
    if ( newzrg.start-newzrg.step>=0 )
	newzrg.start -= newzrg.step;

    newinlrg.stop += newinlrg.step;
    newcrlrg.stop += newcrlrg.step;
    newzrg.stop += newzrg.step;

    const int insz = newinlrg.nrSteps()+1;
    const int crsz = newcrlrg.nrSteps()+1;
    const int zsz = mNINT(newzrg.width()/newzrg.step)+1;
    
    mTryAlloc(res,ImplicitBody);
    if ( !res ) return false;
    
    mDeclareAndTryAlloc(Array3D<float>*,arr,Array3DImpl<float>(insz,crsz,zsz));
    if ( !arr )
    {
	res = 0;
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
    
    Interval<float> zrg;
    StepInterval<int> inlrg( 0, 0, SI().inlStep() );
    StepInterval<int> crlrg( 0, 0, SI().crlStep() );
    
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
    
    const float zscale = SI().zFactor();
    mDeclareAndTryAlloc( Array3D<char>*, chararr, 
	    Array3DImpl<char>( inlrg.nrSteps()+1, crlrg.nrSteps()+1,
			       mNINT(zrg.width()/(zscale*SI().zStep()))+1 ) );
    if ( !chararr )
	return 0; 
   
    chararr->setAll( 1 );

    mDeclareAndTryAlloc(ImplicitBody*,res,ImplicitBody);
    if ( !res )
    {
	delete chararr;
	return 0;
    }
    
    MouseCursorChanger cursorchanger(MouseCursor::Wait);
    
    TypeSet<Coord3> pts = bodypts;
    for ( int idx=0; idx<bodypts.size(); idx++ )
	pts[idx].z *= zscale;

    DAGTetrahedraTree dagtree;
    if ( !dagtree.setCoordList( pts, false ) )
	return 0;
    
    ParallelDTetrahedralator triangulator( dagtree );
    if ( !triangulator.execute(true) )
	return 0;
    
//    PtrMan<Explicit2ImplicitBodyExtracter> extractor =
//	new Explicit2ImplicitBodyExtracter(dagtree,inlrg,crlrg,zrg,*chararr);
    Array3D<float>* arr = new Array3DConv<float,char>(chararr);
    if ( !arr )
    {
	delete chararr;
	delete res;
	return 0;
    }
    
//    if ( !extractor->execute() )
//	res = 0;
    
    res->arr_ = arr;
    res->threshold_ = 0;
    res->inlsampling_.start = inlrg.start;
    res->inlsampling_.step  = inlrg.step;
    res->crlsampling_.start = crlrg.start;
    res->crlsampling_.step  = crlrg.step;
    res->zsampling_.start   = zrg.start;
    res->zsampling_.step    = zscale*SI().zStep();
    
    cursorchanger.restore();
    return res;
}


bool BodyOperator::usePar( const IOPar& par )
{
    if ( (inputbody0_ && !inputbody0_->useBodyPar(par)) ||
	 (inputbody1_ && !inputbody1_->useBodyPar(par)) ||
	 (inputbodyop0_ && !inputbodyop0_->usePar(par)) ||
	 (inputbodyop1_ && !inputbodyop1_->usePar(par)) )
	return false;

    if ( !par.get( sKeyID(), id_ ) ||  !par.get( sKeyChildID(), childid_ ) )
	return false;

    int act;
    if ( !par.get( sKeyAction(), act ) )
	return false;

    action_ = act==0 ? Union : (act==1 ? IntSect : Minus);

    return true;
}


void BodyOperator::fillPar( IOPar& par )
{
    if ( inputbody0_ )  inputbody0_->fillBodyPar(par);
    if ( inputbody1_ )  inputbody1_->fillBodyPar(par);
    if ( inputbodyop0_ )  inputbodyop0_->fillPar(par);
    if ( inputbodyop1_ )  inputbodyop1_->fillPar(par);

    par.set( sKeyID(), id_ );
    par.set( sKeyChildID(), childid_ );
    
    const int act = action_==Union ? 0 : (action_==IntSect ? 1 : 2);
    par.set( sKeyAction(), act );
}


}; //Namespce
