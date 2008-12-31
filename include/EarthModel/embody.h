#ifndef embody_h
#define embody_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embody.h,v 1.3 2008-12-31 09:08:40 cvsranojay Exp $
________________________________________________________________________


-*/

#include "emobject.h"
#include "samplingdata.h"

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


}; // Namespace

#endif
