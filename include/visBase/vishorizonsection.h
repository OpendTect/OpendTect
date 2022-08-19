#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "arrayndimpl.h"
#include "rowcol.h"
#include "visobject.h"
#include "geomelement.h"
#include "vishorizonsectiondef.h"

class BinIDValueSet;
class DataPointSet;

class ZAxisTransform;
class TaskRunner;


namespace Geometry { class BinIDSurface; }
namespace ColTab { class Sequence; class MapperSetup; }
namespace osgGeo { class LayeredTexture; }
namespace osg { class CullStack; }

namespace visBase
{
    class TextureChannel2RGBA;
    class HorizonSectionTile;
    class TextureChannels;
    class HorizonSectionDataHandler;
    class HorizonTextureHandler;
    class HorTilesCreatorAndUpdator;

/*!Horizon geometry is divided into 64*64 pixel tiles. Each tile has it's own
  glue edge to merge into it's neighbors in case of different resolutions. Each
  tile has it's own coordinates and normals, but they all share the same texture
  coordinates since the have the same size. Each tile holds its wireframe. It
  would only turn on wireframe or lines and points depends if you use wireframe
  or not. */

mExpClass(visBase) HorizonSection : public VisualObjectImpl
{
    class TextureCallbackHandler;
    class NodeCallbackHandler;

public:
    static HorizonSection*	create() mCreateDataObj(HorizonSection);

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;
    void			setZAxisTransform(ZAxisTransform*,TaskRunner*);
    ZAxisTransform*		getZAxisTransform() { return zaxistransform_; }

    //Texture information
    void			useChannel(bool);
    int				nrChannels() const;
    void			addChannel();
    void			removeChannel(int);
    void			swapChannels(int,int);

    int				nrVersions(int channel) const;
    void			setNrVersions(int channel,int);
    int				activeVersion(int channel) const;
    void			selectActiveVersion(int channel,int);

    void			setColTabSequence(int channel,
						  const ColTab::Sequence&);
    const ColTab::Sequence*	getColTabSequence(int channel) const;
    void			setColTabMapperSetup(int channel,
						const ColTab::MapperSetup&,
						TaskRunner*);
    const ColTab::MapperSetup*	getColTabMapperSetup(int channel) const;
    const TypeSet<float>*	getHistogram(int channel) const;

    void			setTransparency(int ch,unsigned char yn);
    unsigned char		getTransparency(int ch) const;

    void			getDataPositions(DataPointSet&,
						double zoff /*untransformed*/,
						int sid,TaskRunner*) const;
    void			setTextureData(int channel,const DataPointSet*,
					       int sid,TaskRunner*);
    const BinIDValueSet*	getCache(int channel) const;
    void			inValidateCache(int channel);

    void			setChannels2RGBA(TextureChannel2RGBA*);
				//!Don't share texture processes among sections
    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;
    TextureChannels*		getChannels() const;

    void			setTextureHandler(HorizonTextureHandler&);
				//!Don't share texture handlers among sections
    HorizonTextureHandler&	getTextureHandler();

				//Geometry stuff
    void			setSurface(Geometry::BinIDSurface*,bool conn,
					   TaskRunner*);
    Geometry::BinIDSurface*	getSurface() const	{ return geometry_; }
    StepInterval<int>		displayedRowRange() const;
    StepInterval<int>		displayedColRange() const;
    void			setDisplayRange(const StepInterval<int>&,
						const StepInterval<int>&);
    void			setTextureRange(const StepInterval<int>&,
						const StepInterval<int>&);

    char			nrResolutions() const;
    char			currentResolution() const;
    void			setResolution(char,TaskRunner*);

    void			setWireframeColor(OD::Color col);
    osgGeo::LayeredTexture*	getOsgTexture() const;
    void			updatePrimitiveSets();
    void			turnOsgOn( bool );
    void			enableGeometryTypeDisplay(GeometryType type,
							  bool yn);
				/*!<type 0 is triangle,1 is line,
				2 is point, 3 is wire frame */
    bool			isWireframeDisplayed() const
				{ return wireframedisplayed_ ; }

    void			forceRedraw(bool=true);
    bool			executePendingUpdates();

    int				getNrTiles() const;
    const unsigned char*	getTextureData(int titleidx,int& w,int& h)const;
    bool			getTileTextureCoordinates(
				    int tileidx,TypeSet<Coord>&) const;
    int				getTexturePixelSizeInBits() const;
    void			setUsingNeighborsInIsolatedLine(bool);
    bool			usingNeighborsInIsolatedLine() const;

    void			setLineWidth(int);
    int				getLineWidth() const { return linewidths_; }

protected:
				~HorizonSection();

    friend class		HorizonSectionTile;
    friend class		HorizonTileRenderPreparer;
    friend class		TileResolutionData;
    friend class		HorizonSectionDataHandler;
    friend class		HorizonTextureHandler;
    friend class		HorTilesCreatorAndUpdator;
    friend class		HorizonSectionTileGlue;
    friend class		HorizonSectionTilePosSetup;
    friend class		TileCoordinatesUpdator;
    friend class		HorizonTileResolutionTesselator;
    friend class		DataPointSetFiller;

    void			surfaceChangeCB(CallBacker*);
    void			surfaceChange(const TypeSet<GeomPosID>*,
					      TaskRunner*);
    void			removeZTransform();
    void			updateZAxisVOI();

    void			configSizeParameters();
    void			updateAutoResolution( const osg::CullStack* );
    HorizonSectionTile*		getTile(int idx);
    bool			checkTileIndex(int) const;

    void			setUpdateVar(bool& var,bool yn);
				//! Will trigger redraw request if necessary

    bool			forceupdate_;	// Only set via setUpdateVar(.)


    Geometry::BinIDSurface*	geometry_;
    RowCol			origin_;

    bool			userchangedisplayrg_;
    int				linewidths_ = 1;
    StepInterval<int>		displayrrg_;
    StepInterval<int>		displaycrg_;
    StepInterval<int>		texturerowrg_;
    StepInterval<int>		texturecolrg_;
    Threads::Mutex		updatelock_;
    Threads::SpinLock		spinlock_;

    HorizonSectionDataHandler*	hordatahandler_;
    HorizonTextureHandler*	hortexturehandler_;
    HorTilesCreatorAndUpdator*	hortilescreatorandupdator_;

    NodeCallbackHandler*	nodecallbackhandler_;
    TextureCallbackHandler*	texturecallbackhandler_;
    Threads::Lock		redrawlock_;
    bool			isredrawing_;


    Array2DImpl<HorizonSectionTile*> tiles_;

    const mVisTrans*		transformation_;

    char			lowestresidx_;
    char			desiredresolution_;
    char			nrhorsectnrres_;

    float			rowdistance_;
    float			coldistance_;
    bool			tesselationlock_;

    int				nrcoordspertileside_;
    int				tilesidesize_;
    int				totalnormalsize_;

    int				queueid_;

    TypeSet<int>		spacing_;
    TypeSet<int>		nrcells_;
    TypeSet<int>		normalstartidx_;
    TypeSet<int>		normalsidesize_;

    osg::Group*			osghorizon_;
    ZAxisTransform*		zaxistransform_;

    ObjectSet<HorizonSectionTile> updatedtiles_;
    TypeSet<int>		updatedtileresolutions_;
    bool			wireframedisplayed_;
    bool			useneighbors_;
};

} // namespace visBase
