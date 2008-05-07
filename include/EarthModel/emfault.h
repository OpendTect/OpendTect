#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.31 2008-05-07 12:37:11 cvsnanne Exp $
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
    			FaultGeometry(Fault&);
			~FaultGeometry();

    int			nrSticks(const SectionID&) const;
    int			nrKnots(const SectionID&,int sticknr) const;

    bool		insertStick(const SectionID&, int sticknr,
	    			    const Coord3& pos,const Coord3& editnormal,
				    bool addtohistory);
    bool		removeStick(const SectionID&, int sticknr,
				    bool addtohistory);
    bool		insertKnot(const SectionID&, const SubID&,
	    			   const Coord3& pos,bool addtohistory);
    bool		removeKnot(const SectionID&, const SubID&,
	    			   bool addtohistory);
    
    bool		areSticksVertical(const SectionID&) const;
    const Coord3&	getEditPlaneNormal(const SectionID&,int sticknr) const;

    Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&);
    const Geometry::FaultStickSurface*
			sectionGeometry(const SectionID&) const;

    EMObjectIterator*	createIterator(const SectionID&,
	    			       const CubeSampling* =0) const;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    Geometry::FaultStickSurface*	createSectionGeometry() const;
};



/*!\brief

*/
class Fault : public Surface
{ mDefineEMObjFuncs( Fault );
public:

    FaultGeometry&		geometry();
    const FaultGeometry&	geometry() const;


protected:
    const IOObjContext&		getIOObjContext() const;


    friend class		EMManager;
    friend class		EMObject;
    FaultGeometry		geometry_;
};


}; // Namespace


#endif
