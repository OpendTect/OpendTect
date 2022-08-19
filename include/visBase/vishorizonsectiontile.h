#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

// this header file only be used in the classes related to Horzonsection .
// don't include it in somewhere else !!!

#include "callback.h"
#include "thread.h"
#include "position.h"
#include "color.h"
#include "rowcol.h"
#include "visdata.h"
#include "vishorizonsection.h"

#if defined(visBase_EXPORTS) || defined(VISBASE_EXPORTS)
#include <osg/BoundingBox>
#endif

namespace osg
{
    class CullStack;
    class StateSet;
    class Array;
    class Switch;
    class Geode;
    class Geometry;
    class DrawElementsUShort;
}

namespace visBase
{
    class HorizonSectionTilePosSetup;
    class HorizonSection;
    class TileResolutionData;
    class HorizonSectionTileGlue;
    class Coordinates;
//A tile with 65x65 nodes.

class HorizonSectionTile : CallBacker
{
public:
    HorizonSectionTile(const visBase::HorizonSection&,const RowCol& origin);
    ~HorizonSectionTile();
    char			getActualResolution() const;
    void			updateAutoResolution(const osg::CullStack*);
				/*<Update only when the resolution is -1. */
    void			setPos(int row,int col, const Coord3&);
    void			setPositions(const TypeSet<Coord3>&);
				//Call by the end of each render
				//Makes object ready for render
    void			tesselateResolution(char,bool onlyifabsness);
    void			applyTesselation(char res);
				//!<Should be called from rendering thread
    void			ensureGlueTesselated();
    void			setWireframeColor(OD::Color& color);
    void			setTexture( const Coord& origin,
					    const Coord& opposite );
				//!<Sets origin and opposite in global texture
    void			addTileTesselator( int res );
    void			addTileGlueTesselator();
    ObjectSet<TileResolutionData>& getResolutionData()
				   { return tileresolutiondata_; }
    void			turnOnGlue(bool);
    void			setLineWidth(int);

protected:

    void			setNeighbor(int neighbor,HorizonSectionTile*);
				//!<The neighbor is numbered from 0 to 8

    void			setResolution(char);
				/*!<Resolution -1 means it is automatic. */

    bool			hasDefinedCoordinates(int idx) const;
				/*!<idx is the index of coordinates
				in the highest resolution vertices. */

    const HorizonSectionTile*	getNeighborTile(int idx) const;
				//!<idx is bewteen 0 and 8

    void			enableGeometryTypeDisplay(GeometryType type,
							  bool yn);
				/*!<type 0 is triangle,1 is line,
				2 is point, 3 is wire frame */

    void			updatePrimitiveSets();
    bool			getResolutionNormals(TypeSet<Coord3>&) const;
    bool			getResolutionTextureCoordinates(
							TypeSet<Coord>&) const;
    bool			getResolutionPrimitiveSet( char res,
					     TypeSet<int>&,GeometryType) const;
    bool			getResolutionCoordinates(TypeSet<Coord3>&)const;

    void			dirtyGeometry();

    void			setNrTexCoordLayers(int nrlayers);
    void			initTexCoordLayers();
    osg::Array*			getNormals() { return normals_; }
    osg::Array*			getOsgVertexArray(){ return osgvertices_; }

    friend class		HorizonSectionTilePosSetup;
    friend class		TileTesselator;
    friend class		HorizonSection;
    friend class		TileResolutionData;
    friend class		HorTilesCreatorAndUpdator;
    friend class		HorizonSectionTileGlue;
    friend class		HorizonTextureHandler;
    friend class		TileCoordinatesUpdator;
    friend class		HorizonTileResolutionTesselator;

    void			updateBBox();
    void			buildOsgGeometries();
    void			setActualResolution(char);
    char			getAutoResolution(const osg::CullStack*);
    double			calcGradient(int row,int col,
					     const StepInterval<int>& rcrange,
					     bool isrow);
    void			computeNormal(int nmidx, osg::Vec3&);
    void			initvertices();

    HorizonSectionTile*		neighbors_[9];

    HorizonSectionTileGlue*	righttileglue_;
    HorizonSectionTileGlue*	bottomtileglue_;

    osg::BoundingBox		bbox_;
    const RowCol		origin_;
    const HorizonSection&	hrsection_;
    bool			wireframedisplayed_;

    char			desiredresolution_;
    int				nrdefinedvertices_;

    bool			resolutionhaschanged_;
    bool			needsupdatebbox_;

    int				tesselationqueueid_;
    char			glueneedsretesselation_;
				//!<0 - updated, 1 - needs update, 2 - dont disp

    osg::StateSet*		stateset_;
    TypeSet<int>		txunits_;

    ObjectSet<TileResolutionData>tileresolutiondata_;
    osg::Switch*		osgswitchnode_;
    Threads::Mutex		datalock_;
    bool			updatenewpoint_;
    osg::Vec2f			txorigin_;
    osg::Vec2f			txoppsite_;
    std::vector<osg::Array*>	txcoords_;
    osg::Array*			normals_;
    osg::Array*			osgvertices_;

    const double		cosanglexinl_, sinanglexinl_;
};

} // namespace visBase
