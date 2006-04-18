#ifndef emhorizon2d_h
#define emhorizon2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: emhorizon2d.h,v 1.1 2006-04-18 17:30:16 cvskris Exp $
________________________________________________________________________


-*/

#include "emobject.h"
#include "bufstringset.h"

namespace Geometry { class Horizon2DLine; }

namespace EM
{
class EMManager;

/*!
2d horizons. The horizons is only present along 2d lines, set by addLine. Each
position's subid is formed by RowCol( lineid, tracenr ).getSerialized(). If
multiple z-values per trace is needed, multiple sections can be added. */

class Horizon2D : public EMObject
{
public:
    				Horizon2D( EMManager& );
    const char*		getTypeStr() const;

    int			nrSections() const;
    SectionID		sectionID( int ) const;
    BufferString	sectionName( const SectionID& ) const;
    bool		canSetSectionName() const;
    bool		setSectionName( const SectionID&, const char*,
	    					bool addtohistory );
    int			sectionIndex( const SectionID& ) const;
    bool		removeSection( SectionID, bool hist );
    bool		isAtEdge( const EM::PosID& ) const;

    int			nrLines() const;
    int			lineID(int idx) const;
    const char*		lineName(int id) const;
    int			addLine(const TypeSet<Coord>&,int start,int step,
	    			const char*);
    			/*!<\returns id of new path. */
    void		removeLine( int id );

    EMObjectIterator*	createIterator( const EM::SectionID& ) const;
    Executor*		loader();
    bool		isLoaded() const;
    Executor*		saver();

protected:
    Geometry::Element*	getElementInternal( SectionID );

    const IOObjContext&	getIOObjContext() const;

    BufferStringSet			sectionnames_;
    ObjectSet<Geometry::Horizon2DLine>	sections_;
    TypeSet<int>			sids_;

    BufferStringSet			linenames_;
    TypeSet<int>			lineids_;
};

}; //namespace

#endif
