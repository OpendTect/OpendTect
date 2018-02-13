#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "emobject.h"
#include "arraynd.h"
#include "samplingdata.h"

class TaskRunnerProvider;

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

    Array3D<float>*		arr_;
    float			threshold_;//Any value above threshold is inside
    TrcKeyZSampling		tkzs_; //has same size as arr_
};


/*!
\brief A body that can deliver an implicit body.
*/

mExpClass(EarthModel) Body
{
public:

    virtual ImplicitBody*	createImplicitBody(const TaskRunnerProvider&,
						   bool smooth) const;
    virtual const char*		type() const = 0;

    const IOObjContext&		getBodyContext() const;
    virtual bool		getBodyRange(TrcKeyZSampling&) = 0;

    virtual DBKey		storageID() const	= 0;
    virtual BufferString	storageName()const	= 0;

    virtual void		refBody()	= 0;
				//!<Should be mapped to EM::Object::ref()
    virtual void		unRefBody()	= 0;
				//!<Should be mapped to EM::Object::unRef()
    virtual bool		useBodyPar(const IOPar&)	{ return true; }
				//!<Should be mapped to EM::Object::usePar;
    virtual void		fillBodyPar(IOPar&) const	= 0;
				//!<Should be mapped to EM::Object::fillPar;

protected:
				~Body()		{}
};

} // namespace EM
