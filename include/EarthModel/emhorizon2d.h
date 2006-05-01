#ifndef emhorizon2d_h
#define emhorizon2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon2d.h,v 1.3 2006-05-01 18:04:45 cvskris Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "bufstringset.h"
#include "horizon2dline.h"

namespace EM
{
class EMManager;

class Horizon2Geometry : public RowColSurfaceGeometry
{
public:
    					Horizon2Geometry(Surface&);
    Geometry::Horizon2DLine*		sectionGeometry(const SectionID&);
    const Geometry::Horizon2DLine*	sectionGeometry(const SectionID&) const;

    int				nrLines() const;
    int				lineID(int idx) const;
    const char*			lineName(int id) const;
    int				addLine(const TypeSet<Coord>&,
	    			    int start,int step,
				    const MultiID& lineset,
				    const char* linename);
				/*!<\returns id of new line. */
    void			removeLine( int id );
    bool			isAtEdge(const PosID&) const;
    PosID			getNeighbor(const PosID&,bool nextcol,
	    				    bool retundef=false) const;
    				/*!<\param retundef specifies what to do
				           if no neighbor is found. If it
					   true, it returnes unf, if not
					   it return the id of the undef
					   neighbor. */
					   
    int				getConnectedPos(const PosID& posid,
						TypeSet<PosID>* res) const;

protected:
    Geometry::Horizon2DLine*	createSectionGeometry() const;

    BufferStringSet		linenames_;
    TypeSet<MultiID>		linesets_;
    TypeSet<int>		lineids_;
};

/*!
2d horizons. The horizons is only present along 2d lines, set by addLine. Each
position's subid is formed by RowCol( lineid, tracenr ).getSerialized(). If
multiple z-values per trace is needed, multiple sections can be added. */

class Horizon2D : public Surface
{
public:
    static const char*	typeStr();
    static EMObject*	create(EMManager&);
    static void		initClass(EMManager&);

    const char*		getTypeStr() const;

    Horizon2Geometry&		geometry();
    const Horizon2Geometry&	geometry() const;

protected:
    			Horizon2D( EMManager& );

    const IOObjContext&	getIOObjContext() const;
    Horizon2Geometry		geometry_;
};

}; //namespace

#endif
