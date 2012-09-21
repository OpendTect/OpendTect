#ifndef embodyoperator_h
#define embodyoperator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "arrayndinfo.h"
#include "enums.h"
#include "iopar.h"
#include "multiid.h"
#include "task.h"
#include "trigonometry.h"

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

mClass(EarthModel) BodyOperator
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

    void		setInput(bool body0,const MultiID&);
    void		setInput(bool body0,BodyOperator*);
    			//!<BodyOperator becomes mine
    BodyOperator*	getChildOprt( bool body0 ) const;
    bool		getChildOprt(int freeid,BodyOperator&);

    void		setAction(Action);
    Action		getAction() const		{ return action_; }
    int			getID() const			{ return id_; }
    			/*<If id_ is 0, it is on the top of a tree struct. */
    static int		getFreeID();

    bool		usePar(const IOPar&);
    void		fillPar(IOPar&);

    static const char*	sKeyID()		{ return "ID"; }
    static const char*	sKeyAction()		{ return "Action"; }
    static const char*	sKeyBodyID0()		{ return "BodyID0"; }
    static const char*	sKeyBodyID1()		{ return "BodyID1"; }

protected:

    ImplicitBody*	getOperandBody(bool body0,TaskRunner* tr) const;

    MultiID		inputbody0_;
    BodyOperator*	inputbodyop0_;

    MultiID		inputbody1_;
    BodyOperator*	inputbodyop1_;

    int			id_;
    Action		action_;
};


/*<Given a triangulated body, extract position value on each trace based on 
   threshhold value. The arr's size is based on inlrg, crlrg, zrg. The value at
   each point is the min distance to the body, inside to be negative, and 
   outside to be positive. */
mClass(EarthModel) Expl2ImplBodyExtracter : public ParallelTask
{
public:
			Expl2ImplBodyExtracter( const DAGTetrahedraTree& tree, 
						const StepInterval<int>& inlrg,
						const StepInterval<int>& crlrg,
						const StepInterval<float>& zrg,
						Array3D<float>& arr);
    
    od_int64		nrIterations() const;

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
};


}; // Namespace

#endif

