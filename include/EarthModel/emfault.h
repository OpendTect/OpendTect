#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.36 2008-10-01 03:44:36 cvsnanne Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "faultsticksurface.h"

namespace EM
{
class Fault;

/*!\brief FaultGeometry base class */

class FaultGeometry : public SurfaceGeometry
{
protected:
    			FaultGeometry( Surface& surf )
			    : SurfaceGeometry(surf)	{}
};



/*!\brief Fault base class */

class Fault : public Surface
{
public:
    virtual FaultGeometry&	geometry()			= 0;
    virtual const FaultGeometry& geometry() const
				{ return const_cast<Fault*>(this)->geometry(); }

protected:
    				Fault( EMManager& em )
				    : Surface(em)		{}

    const IOObjContext&		getIOObjContext() const		= 0;
};


} // namespace EM


#endif
