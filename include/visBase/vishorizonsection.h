#ifndef vishorizonsection_h
#define vishorizonsection_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: vishorizonsection.h,v 1.53 2012-07-31 19:27:31 cvskris Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "rowcol.h"
#include "thread.h"
#include "visobject.h"
#include "geomelement.h"

class BinIDValueSet;
class Color;
class DataPointSet;
class SbVec2f;
class SoAction;
class SoCallback;
class SoGetBoundingBoxAction;
class SoShapeHints;
class SoState;
class SoTextureCoordinate2;
class ZAxisTransform;
class TaskRunner;

namespace Geometry { class BinIDSurface; }
namespace ColTab { class Sequence; class MapperSetup; }
namespace osgGeo { class Horizon3DNode; }

namespace visBase
{
    class TextureChannel2RGBA;    
    class HorizonSectionTile;
    class TextureChannels;

/*!Horizon geometry is divided into 64*64 pixel tiles. Each tile has it's own 
  glue edge to merge into it's neighbors in case of different resolutions. Each
  tile has it's own coordinates and normals, but they all share the same texture  coordinates since the have the same size. Each tile holds its wireframe. It 
  would only turn on wireframe or lines and points depends if you use wireframe
  or not. */

mClass HorizonSection : public VisualObjectImpl
{
public:
    static HorizonSection*	create() mCreateDataObj(HorizonSection);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;
    void			setZAxisTransform(ZAxisTransform*,TaskRunner*);

    void			setRightHandSystem(bool);

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

    void			getDataPositions(DataPointSet&,double zoff,
	    					 int sid,TaskRunner*) const;
    void			setTextureData(int channel,const DataPointSet*,
	    				       int sid,TaskRunner*);
    const BinIDValueSet*	getCache(int channel) const;
    void			inValidateCache(int channel);

    void			setChannels2RGBA(TextureChannel2RGBA*);
    TextureChannel2RGBA*	getChannels2RGBA();
    const TextureChannel2RGBA*	getChannels2RGBA() const;
    TextureChannels*		getChannels() const	{ return channels_; }

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
    
    char			nrResolutions() const;
    char			currentResolution() const;
    void			setResolution(int,TaskRunner*);

    void			setWireframeColor(Color col);

protected:
    				~HorizonSection();
    friend class		HorizonSectionTile;			
    friend class		HorizonTileRenderPreparer;			
    void			surfaceChangeCB(CallBacker*);
    void			surfaceChange(const TypeSet<GeomPosID>*,
	    				      TaskRunner*);
    void			removeZTransform();
    void			updateZAxisVOI();

    void			updateTexture(int channel,const DataPointSet*,
	    				      int sectionid);
    void			updateAutoResolution(SoState*,TaskRunner*);
    static void			updateAutoResolution(void*,SoAction*);
    void			updateTileTextureOrigin(const RowCol& texorig);
    void			updateTileArray();
    void			updateBBox(SoGetBoundingBoxAction* action);
    HorizonSectionTile*		createTile(int rowidx,int colidx);

    void			setTextureCoords();
    void			resetAllTiles(TaskRunner*);
    void			updateNewPoints(const TypeSet<GeomPosID>*,
	    					TaskRunner*);
    void			setSizeParameters();

    Geometry::BinIDSurface*	geometry_;
    RowCol			origin_;

    bool			userchangedisplayrg_;
    StepInterval<int>		displayrrg_;
    StepInterval<int>		displaycrg_;
    Threads::Mutex		updatelock_;
    ObjectSet<BinIDValueSet>	cache_;

    TextureChannels*		channels_;
    TextureChannel2RGBA*	channel2rgba_;
    SoTextureCoordinate2*	texturecrds_;

    SoCallback*			callbacker_;
    SoShapeHints*		shapehints_;

    Array2DImpl<HorizonSectionTile*> tiles_;
    bool			usewireframe_;

    const mVisTrans*		transformation_;
    ZAxisTransform*		zaxistransform_;
    int				zaxistransformvoi_; 
    				//-1 not needed by zaxistransform, -2 not set
				
    int				desiredresolution_;
    bool			ismoving_;
    double			cosanglexinl_;
    double			sinanglexinl_;
    float			rowdistance_;
    float			coldistance_;
    Material*			wireframematerial_;

    bool			tesselationlock_;

    int				mNrCoordsPerTileSide;
    int 			mTotalNrCoordsPerTile;
    int 			mTileSideSize;
    int 			mTileLastIdx;
    int 			mTotalNormalSize;
    unsigned char 		mLowestResIdx;
    int 			mHorSectNrRes;

    int*			spacing_;
    int*			nrcells_;
    int*			normalstartidx_;
    int*			normalsidesize_;

    int				tesselationqueueid_;

    osgGeo::Horizon3DNode*	osghorizon_;
    
    static const char*		sKeySectionID()	{ return "Section ID"; }
};

};


#endif
