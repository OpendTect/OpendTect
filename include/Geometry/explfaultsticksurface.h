#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          October 2007
________________________________________________________________________

-*/

#include "geometrymod.h"
#include "indexedshape.h"
#include "enums.h"
#include "position.h"
#include "rowcol.h"
#include "datapack.h"
#include "binidvalue.h"

class DataPointSet;

namespace Geometry
{

class FaultStickSurface;
class ExplFaultStickTexturePositionExtracter;

/*!A triangulated representation of a faultsticksurface */


#define mFltTriProj Geometry::ExplFaultStickSurface::TriProjection


mExpClass(Geometry) ExplFaultStickSurface: public Geometry::IndexedShape,
			      public CallBacker
{
public:
			ExplFaultStickSurface(FaultStickSurface*,
					      float zscale=mUdf(float));
			~ExplFaultStickSurface();

    void		setSceneIdx(int idx)		{ sceneidx_ = idx; }

    bool		needsUpdate() const override	{ return needsupdate_; }

    enum TriProjection	{ None=0, Inline=1, Crossline=2, ZSlice=3 };
			mDeclareEnumUtils(TriProjection);
    TriProjection	triangulateAlg() const		{ return trialg_; }
    void		triangulateAlg(TriProjection);

    void		setSurface(FaultStickSurface*);
    FaultStickSurface*	getSurface()			{ return surface_; }
    const FaultStickSurface* getSurface() const		{ return surface_; }

    void		setZScale( float );

    void		display(bool sticks,bool panels);
    bool		areSticksDisplayed() const    { return displaysticks_; }
    bool		arePanelsDisplayed() const    { return displaypanels_; }

    bool		createsNormals() const override		{ return true; }
    bool		createsTextureCoords() const override	{ return true; }

    void		setMaximumTextureSize(int);
    void		setTexturePowerOfTwo(bool yn);
    void		setTextureSampling(const BinIDValue&);
    const RowCol&	getTextureSize() const;
    void		needUpdateTexture(bool yn);
    bool		needsUpdateTexture() const;

    bool		update(bool forceall,TaskRunner*) override;

    void		addToGeometries(IndexedGeometry*);
    void		removeFromGeometries(const IndexedGeometry*);

    bool		getTexturePositions(DataPointSet&,
					    TaskRunner*);
    const BinIDValue	getBinIDValue() { return texturesampling_; }

    static const char*  sKeyTextureI() { return "Fault texture i column"; }
    static const char*  sKeyTextureJ() { return "Fault texture j column"; }

protected:

    friend		class ExplFaultStickSurfaceUpdater;
    friend		class ExplFaultStickTexturePositionExtracter;

    void		removeAll(bool) override;
    void		insertAll();

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

    void		updateTextureCoords();
    bool		updateTextureSize();
    int			textureColSz(const int panelidx);
    int			sampleSize(const Coord3& p0,const Coord3& p1);
    int			point2LineSampleSz(const Coord3& point,
					   const Coord3& linept0,
					   const Coord3& linept1);
    Coord3		getCoord(int stickidx,int texturerow) const;
    float		getAvgDistance(int stickidx,
				const TypeSet<int>& shift,int extra) const;
    void		shiftStick(int stickidx,int nrunits);
    void		updateStickShifting();
    bool		reTriangulateSurface();
    bool		setProjTexturePositions(DataPointSet& dpset,int id=-1);

    void		addTriangle(IndexedGeometry*,int a,int b,int c);

    bool		displaysticks_;
    bool		displaypanels_;

    FaultStickSurface*	surface_;
    Coord3		scalefacs_;
    int			sceneidx_;

    bool					needsupdate_;
    bool					needsupdatetexture_;
    TriProjection				trialg_;

    ObjectSet<IndexedGeometry>			sticks_;
    ObjectSet<IndexedGeometry>			paneltriangles_;
    ObjectSet<IndexedGeometry>			panellines_;

    TypeSet<int>				texturecolcoords_;
    ObjectSet< TypeSet<int> >			textureknotcoords_;
    int						maximumtexturesize_;
    RowCol					texturesize_;
    bool					texturepot_;
    BinIDValue					texturesampling_;

public:

    bool		getTexturePositions(DataPointSet&,int id,
					    TaskRunner*);
};

};

