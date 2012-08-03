#ifndef embody_h
#define embody_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: embody.h,v 1.14 2012-08-03 13:00:17 cvskris Exp $
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "earthmodelmod.h"
#include "arraynd.h"
#include "emobject.h"
#include "samplingdata.h"

class TaskRunner;

namespace EM
{

/*!Implicit representation of a body. */

mStruct(EarthModel) ImplicitBody
{
    				ImplicitBody();
    				ImplicitBody(const ImplicitBody& nb);
    virtual			~ImplicitBody();
    ImplicitBody		operator =(const ImplicitBody&);

    Array3D<float>*		arr_;
    float 			threshold_;//Any value below threshold is inside
    CubeSampling		cs_; //has same size as arr_
};


/*!A body that can deliver an implicit body. */

mClass(EarthModel) Body
{ 
public:

    virtual ImplicitBody*	createImplicitBody(TaskRunner*,
	    					   bool smooth) const;
    const IOObjContext&		getBodyContext() const;
    virtual bool		getBodyRange(CubeSampling&) = 0;

    virtual MultiID		storageID() const	= 0;
    virtual BufferString	storageName()const	= 0;

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


