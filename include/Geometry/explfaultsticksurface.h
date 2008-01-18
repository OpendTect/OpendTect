#ifndef explfaultsticksurface_h
#define explfaultsticksurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        J.C. Glas
 Date:          October 2007
 RCS:           $Id: explfaultsticksurface.h,v 1.2 2008-01-18 15:39:20 cvskris Exp $
________________________________________________________________________

-*/

#include "indexedshape.h"

class RCol;

namespace Geometry
{

class FaultStickSurface;

/*!A triangulated representation of a faultsticksurface */


class ExplFaultStickSurface: public Geometry::IndexedShape,
			     public CallBacker
{
public:
			ExplFaultStickSurface(FaultStickSurface*);
			~ExplFaultStickSurface();

    void		setSurface(FaultStickSurface*);
    FaultStickSurface*	getSurface()			{ return surface_; }
    const FaultStickSurface* getSurface() const		{ return surface_; }
    
    void		updateAll();
    void		display(bool sticks,bool panels);

protected:
    friend		class ExplFaultStickSurfaceUpdater;    

    void		removeAll();
    void		insertAll();
    void		update();
    
    void		addToGeometries(IndexedGeometry*);
    void		addToGeometries(ObjectSet<IndexedGeometry>&);
    void		removeFromGeometries(const IndexedGeometry* first,
					     int total=1);

    void		emptyStick(int stickidx);
    void		fillStick(int stickidx);
    void		removeStick(int stickidx);
    void		insertStick(int stickidx);

    void		emptyPanel(int panelidx);
    void		fillPanel(int panelidx);
    void		removePanel(int panelidx);
    void		insertPanel(int panelidx);

    void		calcNormals(IndexedGeometry&,bool mirrored);

    void		surfaceChange(CallBacker*);


    bool		displaysticks_;
    bool		displaypanels_;

    FaultStickSurface*	surface_;

    ObjectSet<IndexedGeometry>			sticks_;
    ObjectSet<ObjectSet<IndexedGeometry> >	panels_;
};

};

#endif
