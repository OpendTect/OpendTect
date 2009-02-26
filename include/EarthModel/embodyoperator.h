#ifndef embodyoperator_h
#define embodyoperator_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		Feb 2009
 RCS:		$Id: embodyoperator.h,v 1.2 2009-02-26 15:09:04 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "arrayndinfo.h"
#include "enums.h"
#include "iopar.h"
#include "task.h"

class TaskRunner;
class Coord3;
class Plane3;
class DAGTetrahedraTree;

template<class T> class Array3D;
template <class T> class TypeSet;

namespace EM
{

class Body;    
class ImplicitBody;    

/*!Operators for implicit body. Each BodyOperator has two children, either a
   Body or a BodyOperator. */

mClass BodyOperator
{ 
public:
    			BodyOperator();
			~BodyOperator();
    enum Action		{ Union, IntSect, Minus };
    			DeclareEnumUtils(Action);

    bool		isOK() const;

    bool		createImplicitBody(ImplicitBody*&,TaskRunner*) const;
    			/*<Make a body using b0 u/n/- b1. If use "Minus"
			   b0-b1,it means the body part in b0 but not in b1. 
			   Set two inputs and one Action before use it. */

    ImplicitBody*	createImplicitBody(const TypeSet<Coord3>& bodypts,
					   TaskRunner* tr=0) const;
    			/*<This is an independent function which creates an 
			   implicite body with position value -1,0,1 depends on
			   the position is inside, on or outside the 
			   triangulated body for a set of random points. Users
			   don't have to set any action or body. */

    void		setInput(bool body0,Body*);
    			//!<Body becomes mine
    void		setInput(bool body0,BodyOperator*);
    			//!<BodyOperator becomes mine

    Action		getAction() const		{ return action_; }
    void		setAction(Action);

    void		setParent(BodyOperator*);
    			/*<The parent gives us the id of myself. */
    int			getID() const			{ return id_; }
    			/*<If id_ is 0, it is on the top of a tree struct. */
    int			getChildID() const		{ return childid_; }
    			/*<If is -1, it does not have BodyOperator child, 
			   otherwise, childid = id+1. */

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&);

    static const char*	sKeyID()		{ return "ID"; }
    static const char*	sKeyChildID()		{ return "ChildID"; }
    static const char*	sKeyAction()		{ return "Action"; }

protected:

    Body*		inputbody0_;
    BodyOperator*	inputbodyop0_;

    Body*		inputbody1_;
    BodyOperator*	inputbodyop1_;

    int			id_;
    int			childid_;

    Action		action_;
};


/*<Given a triangulated body, extract position value on each trace based on 
   threshhold value. The arr's size is based on inlrg, crlrg, zrg. The value at
   each point is the min distance to the body, inside to be negative, and 
   outside to be positive. */
mClass Expl2ImplBodyExtracter : public ParallelTask
{
public:
			Expl2ImplBodyExtracter( const DAGTetrahedraTree& tree, 
						const StepInterval<int>& inlrg,
						const StepInterval<int>& crlrg,
						const StepInterval<float>& zrg,
						Array3D<float>& arr);
    
    od_int64		totalNr() const;

private:

    bool		doPrepare(int nrthreads);
    bool		doWork(od_int64,od_int64,int);

    const DAGTetrahedraTree&	tree_;    
    StepInterval<float>		zrg_;
    const StepInterval<int>&	inlrg_;
    const StepInterval<int>&	crlrg_;
    Array3D<float>&		arr_;
    TypeSet<int>		tri_;
    TypeSet<Plane3>		planes_;
    TypeSet<Coord3>		pts_;
};


}; // Namespace

#endif
