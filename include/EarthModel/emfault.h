#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.39 2009-07-22 16:01:14 cvsbert Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "faultsticksurface.h"

namespace EM
{
class Fault;

/*!\brief FaultGeometry base class */

mClass FaultGeometry : public SurfaceGeometry
{
public:
    virtual bool	insertStick(const SectionID&,int sticknr,int firstcol,
				    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory)	{ return false; }
    virtual bool        insertKnot(const SectionID&,const SubID&,
	    			   const Coord3& pos,bool addtohistory)
			{ return false; }

protected:
    			FaultGeometry( Surface& surf )
			    : SurfaceGeometry(surf)	{}
};



/*!\brief Fault base class */

mClass Fault : public Surface
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
