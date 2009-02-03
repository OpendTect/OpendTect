#ifndef embody_h
#define embody_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embody.h,v 1.4 2009-02-03 23:00:21 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "emobject.h"
#include "samplingdata.h"

#include "delaunay3d.h"
#include "mousecursor.h"
#include "survinfo.h"

template <class T> class Array3D;
class TaskRunner;

namespace EM
{

/*!Implicit representation of a body. */

mStruct ImplicitBody
{
    			ImplicitBody();
    virtual		~ImplicitBody();

    Array3D<float>*	arr_;
    float 		threshold_;
    SamplingData<int>	inlsampling_;
    SamplingData<int>	crlsampling_;
    SamplingData<float>	zsampling_;
};

/*!A body that can deliver an implicit body. */

mClass Body
{ 
public:

    virtual ImplicitBody*	createImplicitBody(TaskRunner*) const = 0;
    				//!<Returned object becomes caller.
    const IOObjContext&		getBodyContext() const;
};


/*<Create an implicite body with position value -1,0,1 depends on the position
   is inside, on or outside the triangulated body for a set of random points.*/
mClass ImplicitBodyCreater
{
public:    
    			ImplicitBodyCreater()	{}
			~ImplicitBodyCreater()	{}

    ImplicitBody*	createImplicitBody( const TypeSet<Coord3>& bodypts,
	    				    TaskRunner* tr=0 ) const
    			{
			    if ( bodypts.size()<3 )
				return 0;

			    Interval<float> zrg;
			    StepInterval<int> inlrg( 0, 0, SI().inlStep() );
			    StepInterval<int> crlrg( 0, 0, SI().crlStep() );
			    
			    for ( int idx=0; idx<bodypts.size(); idx++ )
			    {
				const BinID bid = 
				    SI().transform( bodypts[idx].coord() );

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
			    
			    mDeclareAndTryAlloc( Array3D<char>*, chararr,
				    Array3DImpl<char>( inlrg.nrSteps()+1,
					crlrg.nrSteps()+1,
					mNINT(zrg.width()/SI().zStep())+1 ) );
			    if ( !chararr )
				return 0;
			    
			    mDeclareAndTryAlloc(ImplicitBody*,res,ImplicitBody);
			    if ( !res )
			    {
				delete chararr;
				return 0;
			    }
			    
			    memset( chararr->getData(), 1, 
				    sizeof(char)*chararr->info().getTotalSz() );
			    
			    MouseCursorChanger cursorchanger(MouseCursor::Wait);

			    TypeSet<Coord3> pts;
			    const float zscale = SI().zFactor();
			    for ( int idx=0; idx<bodypts.size(); idx++ )
			    {
				pts += bodypts[idx];
				pts[idx].z *= zscale;
			    }
			    
			    DAGTetrahedraTree dagtree;
			    if ( !dagtree.setCoordList( pts, false ) )
				return 0;
			    
			    ParallelDTetrahedralator triangulator( dagtree );
			    if ( !triangulator.execute(true) )
				return 0;

			    PtrMan<Explicit2ImplicitBodyExtracter> extractor =
				new Explicit2ImplicitBodyExtracter( dagtree, 
					inlrg, crlrg, zrg, *chararr );
			    Array3D<float>* arr =
				new Array3DConv<float,char>(chararr);
			    if ( !arr )
			    {
				delete chararr;
				delete res;
				return 0;
			    }
			    
			    res->arr_ = arr;
			    res->threshold_ = 0;
			    res->inlsampling_.start = inlrg.start;
			    res->inlsampling_.step = inlrg.step;
			    res->crlsampling_.start = crlrg.start;
			    res->crlsampling_.step = crlrg.step;
			    res->zsampling_.start = zrg.start;
			    res->zsampling_.step = SI().zStep();
			    
			    if ( !extractor->execute() )
				res = 0;
			    
			    cursorchanger.restore();
			    return res;
			}	
};


}; // Namespace

#endif
