#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.26 2006-04-27 15:29:13 cvskris Exp $
________________________________________________________________________


-*/
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "cubicbeziersurface.h"


template <class T> class SortedList;

namespace EM
{
class Fault;

class FaultGeometry : public SurfaceGeometry
{
public:
    			FaultGeometry( Fault& );
			~FaultGeometry();

    Geometry::CubicBezierSurface*
			sectionGeometry(const EM::SectionID&);

protected:
    Geometry::CubicBezierSurface*	createSectionGeometry() const;
};



/*!\brief

*/
class Fault : public Surface
{
public:
    static const char*		typeStr(); 
    static EMObject*		create( EMManager& );
    static void			initClass(EMManager&);

    const char*			getTypeStr() const { return typeStr(); }

    FaultGeometry&		geometry();

protected:
				Fault(EMManager&);

    const IOObjContext&		getIOObjContext() const;


    friend class		EMManager;
    friend class		EMObject;
    FaultGeometry		geometry_;
};


}; // Namespace


#endif
