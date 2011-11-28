#ifndef embody_h
#define embody_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embody.h,v 1.10 2011-11-28 21:50:06 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "arraynd.h"
#include "emobject.h"
#include "samplingdata.h"

class TaskRunner;

namespace EM
{

/*!Implicit representation of a body. */

mStruct ImplicitBody
{
    				ImplicitBody();
    				ImplicitBody(const ImplicitBody& nb)
				{ *this = nb; }
    virtual			~ImplicitBody();

    Array3D<float>*		arr_;
    float 			threshold_;
    				//Any value above threshold is inside
    SamplingData<int>		inlsampling_;
    SamplingData<int>		crlsampling_;
    SamplingData<float>		zsampling_;

    ImplicitBody		operator =(const ImplicitBody&);
};


/*!A body that can deliver an implicit body. */

mClass Body
{ 
public:

    virtual ImplicitBody*	createImplicitBody(TaskRunner*,
	    					   bool smooth) const;
    const IOObjContext&		getBodyContext() const;

    virtual void		refBody()	= 0;
    				//!<Should be mapped to EMObject::ref()
    virtual void		unRefBody()	= 0;
    				//!<Should be mapped to EMObject::unRef()
    virtual bool		useBodyPar(const IOPar&)	{ return true; }
    				//!<Should be mapped to EMObject::usePar;
    virtual void                fillBodyPar(IOPar&) const	= 0;
    				//!<Should be mapped to EMObject::fillPar;
protected:
    				~Body()		{}
};


}; // Namespace

#endif
