#ifndef geeditor_h
#define geeditor_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: geeditor.h,v 1.4 2008-12-25 11:55:38 cvsranojay Exp $
________________________________________________________________________

-*/

#include "geomelement.h"

namespace Geometry
{

class TrackPlane;


mClass ElementEditor : public CallBacker
{
public:
    			ElementEditor( Geometry::Element& element );
    virtual		~ElementEditor();

    const Element&	getElement() const { return element; }
    Element&		getElement() { return element; }

    virtual void	getEditIDs( TypeSet<GeomPosID>& ) const;
    virtual Coord3	getPosition( GeomPosID ) const;
    virtual bool	setPosition( GeomPosID, const Coord3& );

    virtual bool	mayTranslate1D( GeomPosID ) const;
    virtual Coord3	translation1DDirection( GeomPosID ) const;

    virtual bool	mayTranslate2D( GeomPosID ) const;
    virtual Coord3	translation2DNormal( GeomPosID ) const;

    virtual bool	mayTranslate3D( GeomPosID ) const;

    virtual bool	maySetNormal( GeomPosID ) const;
    virtual Coord3	getNormal( GeomPosID ) const;
    virtual bool	setNormal( GeomPosID, const Coord3& );

    virtual bool	maySetDirection( GeomPosID ) const;
    virtual Coord3	getDirectionPlaneNormal( GeomPosID ) const;
    virtual Coord3	getDirection( GeomPosID ) const;
    virtual bool	setDirection( GeomPosID, const Coord3& );

    Notifier<ElementEditor>	editpositionchange;
    				/*!<Won't trigger on position-changes, but
				    when new edit positions are avaliable or
				    editpositions has been removed */

protected:

    Geometry::Element&	element;
};

};

#endif

