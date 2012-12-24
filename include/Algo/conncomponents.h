#ifndef conncomponents_h
#define conncomponents_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bo Zhang/Yuancheng Liu
 Date:          July 2012
 RCS:           $Id$
________________________________________________________________________


-*/


#include "algomod.h"
#include "factory.h"


template <class T> class Array2D;
template <class T> class Array2DSlice;
template <class T> class Array3D;
template <class T> class TypeSet;

class TaskRunner;

/*!
\ingroup Algo
\brief Classify connected components of a binarized array 2D, components are
sorted in size. User could get the best quadratic fit for the component if
needed.
*/

mClass(Algo) ConnComponents
{
public:    

    				ConnComponents(const Array2D<bool>&);
				~ConnComponents();

    void			compute(TaskRunner* tr=0); 

    int				nrComponents() const; 
    const TypeSet<int>*		getComponent(int compidx);
    const Array2D<int>*		getLabel() const	{ return label_; }
    void			trimCompBranches(TypeSet<int>& comp);
    static void			trimCompBranches(TypeSet<int>& comp,int sz1);
    static void			getCompSticks(TypeSet<int>& comp,int sz1,
	    				int allowgapsz, int minsticksz,
					TypeSet<TypeSet<int> >& sticks);
    				/*Will change comp, make copy before call this*/

    float			overLapRate(int componentidx);
    				/*Minimum rate of all dimensions.*/

protected:

    void			classifyMarks(Array2D<int>& mark);
    void			setMark(Array2D<int>& r,int source,int newval);

    const Array2D<bool>&	input_;
    Array2D<int>*		label_;
    TypeSet< TypeSet<int> >	components_;
    TypeSet<int>		sortedindex_;
};


/*!
\ingroup Algo
\brief Classify connected components of a binarized array 3D,
components are sorted in size.
*/

mClass(Algo) ConnComponents3D 
{
public:    

    				ConnComponents3D(const Array3D<bool>&);
				~ConnComponents3D();

    void			compute(TaskRunner* tr=0); 

    struct VPos	{
				VPos() : i(-1),j(-1),k(-1) {}
				int i;int j;int k;
    		};
    int				nrComponents() const; 
    const ObjectSet<VPos>*	getComponent(int compidx);
    				/*<Comp size is sorted descending. */

protected:

    void			addToComponent(
	    			const TypeSet<TypeSet<TypeSet<int> > >& comps, 
				int sliceidx,int compidx,
				TypeSet<TypeSet<unsigned char> >& usedcomps,
				ObjectSet<VPos>& rescomp);

    const Array3D<bool>&	input_;
    ObjectSet< ObjectSet<VPos> > components_;
    TypeSet<int>		sortedindex_;
};


#endif

