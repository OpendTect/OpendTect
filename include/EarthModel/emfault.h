#ifndef emfault_h
#define emfault_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		9-04-2002
 RCS:		$Id: emfault.h,v 1.21 2005-01-06 09:38:14 kristofer Exp $
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
class Fault : public EM::Surface
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



class FaultGeometry : public EM::SurfaceGeometry
{
public:
    			FaultGeometry( EM::Fault& );
			~FaultGeometry();

    virtual EM::PosID	getNeighbor( const EM::PosID& posid,
				     const RowCol& dir ) const;
    int			findPos(const EM::SectionID&,const Interval<float>& x,
				const Interval<float>& y,
				const Interval<float>& z,
				TypeSet<EM::PosID>* res) const;
    bool		isHidden( const EM::PosID& ) const;
    bool		isHidden( const EM::SectionID&, const RowCol&) const;
    void		setHidden( const EM::PosID&, bool yn,bool addtohistory);
    void		setHidden( const EM::SectionID&, const RowCol&,
	    			   bool yn,bool addtohistory);
    bool		insertHiddenColumn( const EM::SectionID&, int col );
    Coord3		getPos(const SectionID& section, const RowCol&) const;

    void		updateHiddenPos();

protected:
    bool			createFromStick(const TypeSet<Coord3>&,
	    					const SectionID&,float);
    Geometry::MeshSurface*	createSectionSurface(const SectionID&) const;

    static int			hiddenAttrib() { return 3; }
    				//TODO: Make better implementation
};


}; // Namespace


#endif
