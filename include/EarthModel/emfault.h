#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.22 2005-03-10 11:47:17 cvskris Exp $
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
    static EMObject*		create( const ObjectID&, EMManager& );
    static void			initClass(EMManager&);

    const char*			getTypeStr() const { return typeStr(); }

protected:
				Fault(EMManager&,const ObjectID&);

    const IOObjContext&		getIOObjContext() const;


    friend class		EMManager;
    friend class		EMObject;

};



class FaultGeometry : public SurfaceGeometry
{
public:
    			FaultGeometry( Fault& );
			~FaultGeometry();

    virtual PosID	getNeighbor( const PosID& posid,
				     const RowCol& dir ) const;
    int			findPos(SectionID,const Interval<float>& x,
				const Interval<float>& y,
				const Interval<float>& z,
				TypeSet<PosID>* res) const;
    bool		isHidden( const PosID& ) const;
    bool		isHidden( SectionID, const RowCol&) const;
    void		setHidden( const PosID&, bool yn,bool addtohistory);
    void		setHidden( SectionID, const RowCol&,
	    			   bool yn,bool addtohistory);
    bool		insertHiddenColumn( SectionID, int col );
    Coord3		getPos(SectionID section, const RowCol&) const;

    void		updateHiddenPos();

protected:
    Geometry::ParametricSurface*	createSectionSurface() const;

    static int			hiddenAttrib() { return 3; }
    				//TODO: Make better implementation
};


}; // Namespace


#endif
