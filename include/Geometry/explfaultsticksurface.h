#ifndef explfaultsticksurface_h
#define explfaultsticksurface_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        J.C. Glas
 Date:          October 2007
 RCS:           $Id: explfaultsticksurface.h,v 1.7 2008-05-16 14:53:57 cvskris Exp $
________________________________________________________________________

-*/

#include "indexedshape.h"
#include "position.h"
#include "rowcol.h"
#include "datapack.h"

class RCol;

namespace Geometry
{

class FaultStickSurface;

/*!A triangulated representation of a faultsticksurface */


class ExplFaultStickSurface: public Geometry::IndexedShape,
			     public CallBacker
{
public:
			ExplFaultStickSurface(FaultStickSurface*,float zscale);
    			~ExplFaultStickSurface();

    bool		needsUpdate() const 		{ return needsupdate_; }

    void		setSurface(FaultStickSurface*);
    FaultStickSurface*	getSurface()			{ return surface_; }
    const FaultStickSurface* getSurface() const		{ return surface_; }

    void		setZScale( float );

    void		display(bool sticks,bool panels);
    bool		areSticksDisplayed() const    { return displaysticks_; }
    bool		arePanelsDisplayed() const    { return displaypanels_; }

    void		setMaximumTextureSize(const RowCol&);
    void		setTexturePowerOfTwo(bool yn);
    void		setTextureSampling(const BinIDValue&);
    const RowCol&	getTextureSize() const;

    DataPack::ID	getDataPointSet() const;

protected:
    friend		class ExplFaultStickSurfaceUpdater;    

    void		removeAll();
    void		insertAll();
    bool		update(bool forceall,TaskRunner*);
    
    void		addToGeometries(IndexedGeometry*);
    void		removeFromGeometries(const IndexedGeometry*);

    void		emptyStick(int stickidx);
    void		fillStick(int stickidx);
    void		removeStick(int stickidx);
    void		insertStick(int stickidx);

    void		emptyPanel(int panelidx);
    void		fillPanel(int panelidx);
    void		removePanel(int panelidx);
    void		insertPanel(int panelidx);

    void		surfaceChange(CallBacker*);
    void		surfaceMovement(CallBacker*);

    bool		updateTextureSize() 			{ return true; }

    bool		displaysticks_;
    bool		displaypanels_;

    FaultStickSurface*	surface_;
    Coord3		scalefacs_;

    bool					needsupdate_;

    ObjectSet<IndexedGeometry>			sticks_;
    TypeSet<int>				texturerows_;
    ObjectSet<IndexedGeometry>			paneltriangles_;
    ObjectSet<IndexedGeometry>			panellines_;

    RowCol					maximumtexturesize_;
    RowCol					texturesize_;
    bool					textrurepot_;
    BinIDValue					texturesampling_;
};

};

#endif
