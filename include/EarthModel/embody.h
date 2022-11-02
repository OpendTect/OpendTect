#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "arraynd.h"
#include "emobject.h"

class TaskRunner;

/*!\brief Earth Model objects like horizons, faults, fault-sticks and bodies.*/

namespace EM
{

/*!
\brief Implicit representation of a body.
*/

mExpClass(EarthModel) ImplicitBody
{
public:
				ImplicitBody();
				ImplicitBody(const ImplicitBody& nb);
    virtual			~ImplicitBody();

    ImplicitBody		operator =(const ImplicitBody&);

    Array3D<float>*		arr_			= nullptr;
    float			threshold_		= mUdf(float);
				//Any value above threshold is inside
    TrcKeyZSampling		tkzs_; //has same size as arr_
};


/*!
\brief A body that can deliver an implicit body.
*/

mExpClass(EarthModel) Body
{
public:
    virtual			~Body();

    virtual ImplicitBody*	createImplicitBody(TaskRunner*,
						   bool smooth) const;
    virtual const char*		type() const = 0;

    const IOObjContext&		getBodyContext() const;
    virtual bool		getBodyRange(TrcKeyZSampling&) = 0;

    virtual MultiID		storageID() const	= 0;
    virtual BufferString	storageName()const	= 0;

    virtual void		refBody()	= 0;
				//!<Should be mapped to EMObject::ref()
    virtual void		unRefBody()	= 0;
				//!<Should be mapped to EMObject::unRef()
    virtual bool		useBodyPar(const IOPar&)	{ return true; }
				//!<Should be mapped to EMObject::usePar;
    virtual void		fillBodyPar(IOPar&) const	= 0;
				//!<Should be mapped to EMObject::fillPar;

protected:
				Body();
};

} // namespace EM
