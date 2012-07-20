#ifndef conncomponents_h
#define conncomponents_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bo Zhang/Yuancheng Liu
 Date:          July 2012
 RCS:           $Id: conncomponents.h,v 1.1 2012-07-20 20:05:01 cvsyuancheng Exp $
________________________________________________________________________


-*/


#include "factory.h"


template <class T> class Array2D;
template <class T> class TypeSet;

class TaskRunner;

/*Classify connected components of a binarized array 2D, components are sorted 
  in size. User could get the best quadratic fit for the compoment if needed. */

mClass ConnComponents
{
public:    

    				ConnComponents(const Array2D<bool>&);
				~ConnComponents()	{}

    void			compute(TaskRunner* tr=0); 

    int				nrComponents() const; 
    const TypeSet<int>*		getComponent(int compidx);

    bool			hasTrifurcation(const TypeSet<int>& component);
    float			overLapRate(int componentidx);
    				/*Minimum rate of all dimensions.*/

protected:

    bool			quadraticFit(int compidx,TypeSet<int>&res);
    				/*disabled for now*/
    void			classifyMarks(Array2D<int>& mark);
    void			setMark(Array2D<int>& r,int source,int newval);
    const Array2D<bool>&	input_;
    TypeSet< TypeSet<int> >	components_;
    TypeSet<int>		sortedindex_;
};



#endif
