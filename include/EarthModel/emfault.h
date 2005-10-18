#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.25 2005-10-18 20:16:43 cvskris Exp $
________________________________________________________________________


-*/
#include "emsurface.h"
#include "emsurfacegeometry.h"

template <class T> class SortedList;
namespace Geometry { class MeshSurface; };

namespace EM
{

/*!\brief

*/
class Fault : public Surface
{
public:
    static const char*		typeStr(); 
    static EMObject*		create( EMManager& );
    static void			initClass(EMManager&);

    const char*			getTypeStr() const { return typeStr(); }

protected:
				Fault(EMManager&);

    const IOObjContext&		getIOObjContext() const;


    friend class		EMManager;
    friend class		EMObject;

};



class FaultGeometry : public SurfaceGeometry
{
public:
    			FaultGeometry( Fault& );
			~FaultGeometry();

protected:
    Geometry::ParametricSurface*	createSectionSurface() const;
};


}; // Namespace


#endif
