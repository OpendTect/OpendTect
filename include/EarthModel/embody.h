#ifndef embody_h
#define embody_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embody.h,v 1.1 2008-09-04 13:22:42 cvskris Exp $
________________________________________________________________________


-*/

#include "emobject.h"
#include "samplingdata.h"

template <class T> class Array3D;
class TaskRunner;

namespace EM
{

/*!Implicit representation of a body. */

struct ImplicitBody
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

class Body : public EMObject
{ 
public:

    virtual ImplicitBody*	createImplicitBody(TaskRunner*) const;
    				//!<Returned object becomes caller's.
protected:
    				Body(EMManager&);
};


}; // Namespace

#endif
