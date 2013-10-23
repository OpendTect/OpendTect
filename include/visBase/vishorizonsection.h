#ifndef vishorizonsection_h
#define vishorizonsection_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "arrayndimpl.h"
#include "rowcol.h"
#include "visobject.h"
#include "geomelement.h"

class BinIDValueSet;
class DataPointSet;

class ZAxisTransform;
class TaskRunner;


namespace Geometry { class BinIDSurface; }
namespace ColTab { class Sequence; class MapperSetup; }
namespace osgGeo { class LayeredTexture;  }
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
public:
    static HorizonSection*	create() mCreateDataObj(HorizonSection);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    void			setZAxisTransform(ZAxisTransform*,TaskRunner*);

    //Texture information
    void                        useChannel(bool);
    int                         nrChannels() const;
    void                        addChannel();
    void                        removeChannel(int);
    void                        swapChannels(int,int);

    int                         nrVersions(int channel) const;
    void                        setNrVersions(int channel,int);
    int                         activeVersion(int channel) const;
    void                        selectActiveVersion(int channel,int);

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
    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;
    TextureChannels*		getChannels() const;	//{ return channels_; }

    				//Geometry stuff
    void			setSurface(Geometry::BinIDSurface*,bool conn,
	    				   TaskRunner*);
    Geometry::BinIDSurface*	getSurface() const	{ return geometry_; }
    StepInterval<int>		displayedRowRange() const;
    StepInterval<int>		displayedColRange() const;
    void			setDisplayRange(const StepInterval<int>&,
	    					const StepInterval<int>&);

    void			useWireframe(bool);
    bool			usesWireframe() const;
    
    char	 		nrResolutions() const;
    char 			currentResolution() const;
    void			setResolution(char,TaskRunner*);

    void			setWireframeColor(Color col);
    osgGeo::LayeredTexture*	getOsgTexture() const;
    void			updatePrimitiveSets();
    void			turnOsgOn( bool );
    void			setDisplayGeometryType( int type );
    void			updateTiles();

protected:
    				~HorizonSection();
    friend class		HorizonSectionTile;			
    friend class		HorizonTileRenderPreparer;
    friend class		TileResolutionData;
    friend class		HorizonSectionOsgCallBack;
    friend class		HorizonSectionDataHandler;
    friend class                HorizonTextureHandler;
    friend class		HorTilesCreatorAndUpdator;
    friend class		HorizonSectionTileGlue;

    void			surfaceChangeCB(CallBacker*);
    void			surfaceChange(const TypeSet<GeomPosID>*,
	    				      TaskRunner*);
    void			removeZTransform();
    void			updateZAxisVOI();

    void			configSizeParameters();
    void			updateAutoResolution( const osg::CullStack* );

    Geometry::BinIDSurface*	geometry_;
    RowCol			origin_;

    bool			userchangedisplayrg_;
    StepInterval<int>		displayrrg_;
    StepInterval<int>		displaycrg_;
    Threads::Mutex		updatelock_;
    Threads::SpinLock		spinlock_;

    HorizonSectionDataHandler*  hordatahandler_;
    HorizonTextureHandler*	hortexturehandler_;
    HorTilesCreatorAndUpdator*	hortilescreatorandupdator_;

    Array2DImpl<HorizonSectionTile*> tiles_;
    bool			usewireframe_;

    const mVisTrans*		transformation_;
				
    char			lowestresidx_;
    char			desiredresolution_;
    char 			nrhorsectnrres_;

    float			rowdistance_;
    float			coldistance_;
    bool			tesselationlock_;

    int				nrcoordspertileside_;
    int 			tilesidesize_;
    int 			totalnormalsize_;

    TypeSet<int>		spacing_;
    TypeSet<int>		nrcells_;
    TypeSet<int>		normalstartidx_;
    TypeSet<int>		normalsidesize_;

    osg::Group*		    	osghorizon_;
    bool			forceupdate_;

    ObjectSet<HorizonSectionTile> updatedtiles_;
    TypeSet<int>		  updatedtileresolutions_;

};

};


#endif
