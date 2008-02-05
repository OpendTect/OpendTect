#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.28 2008-02-05 21:46:15 cvskris Exp $
________________________________________________________________________


-*/
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "faultsticksurface.h"


template <class T> class SortedList;

namespace EM
{
class Fault;

class FaultGeometry : public SurfaceGeometry
{
public:
    			FaultGeometry( Fault& );
			~FaultGeometry();
    int			nrSticks() const;
    void		insertStick(const EM::SectionID&, int sticknr,
	    			    const Coord3&,bool addtohistory);
    void		removeStick(const EM::SectionID&, int sticknr);

    Geometry::FaultStickSurface*
			sectionGeometry(const EM::SectionID&);
    EMObjectIterator*	createIterator(const SectionID&,
	    			       const CubeSampling*) const;

protected:
    Geometry::FaultStickSurface*	createSectionGeometry() const;
};



/*!\brief

*/
class Fault : public Surface
{ mDefineEMObjFuncs( Fault );
public:

    FaultGeometry&		geometry();

protected:
    const IOObjContext&		getIOObjContext() const;


    friend class		EMManager;
    friend class		EMObject;
    FaultGeometry		geometry_;
};


}; // Namespace


#endif
