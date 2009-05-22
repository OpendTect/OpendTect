#ifndef vishorizonsection_h
#define vishorizonsection_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: vishorizonsection.h,v 1.17 2009-05-22 21:43:52 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "position.h"
#include "rowcol.h"
#include "visobject.h"

class BinIDValueSet;
class Color;
class SbBox3f;
class SbVec2f;
class SoAction;
class SoCallback;
class SoGetBoundingBoxAction;
class SoGroup;
class SoState;
class SoIndexedLineSet;
class SoDGBIndexedPointSet;
class SoIndexedTriangleStripSet;
class SoNormal;
class SoTextureComposer;
class SoTextureCoordinate2;
class ZAxisTransform;

namespace Geometry { class BinIDSurface; }
namespace ColTab { class Sequence; struct MapperSetup; }

#define mHorSectNrRes		6

namespace visBase
{

class VisColorTab;    
class ColTabTextureChannel2RGBA;    
class Coordinates;
class HorizonSectionTile;
class TextureChannels;
class Texture2;
class TileResolutionTesselator;

/*!Horizon geometry is divided into 64*64 pixel tiles. Each tile has it's own 
  glue edge to merge into it's neighbors in case of different resolutions. Each
  tile has it's own coordinates and normals, but they all share the same texture  coordinates since the have the same size. Each tile holds its wireframe. It 
  would only turn on wireframe or lines and points depends if you use wireframe
  or not. */

mClass HorizonSection : public VisualObjectImpl
{
public:
    static HorizonSection*	create() mCreateDataObj(HorizonSection);

    void			setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();
    void			setZAxisTransform(ZAxisTransform*);

    void			allowShading(bool);
    
    void                        useChannel(bool);
    int                         nrChannels() const;
    int				maxNrChannels() const;
    void                        addChannel();
    void                        removeChannel(int);
    void                        swapChannels(int,int);

    void			enableChannel(int channel,bool yn);
    bool			isChannelEnabled(int channel) const;

    int                         nrVersions(int channel) const;
    void                        setNrVersions(int channel,int);
    int                         activeVersion(int channel) const;
    void                        selectActiveVersion(int channel,int);

    void			setSurface(Geometry::BinIDSurface*,bool conn);
    Geometry::BinIDSurface*	getSurface() const	{ return geometry_; }

    void			useWireframe(bool);
    bool			usesWireframe() const;
    
    char			nrResolutions() const { return mHorSectNrRes; }
    char			currentResolution() const;
    void			setResolution(int);

    void			setWireframeColor(Color col);
    void			setColTabSequence(int channel,
	    					  const ColTab::Sequence&);
    const ColTab::Sequence*	getColTabSequence(int channel) const;
    void			setColTabMapperSetup(int channel,
						const ColTab::MapperSetup&);
    const ColTab::MapperSetup*	getColTabMapperSetup(int channel) const;

    void			setTransparency(int ch,unsigned char yn);
    unsigned char		getTransparency(int ch) const;

    void			getDataPositions(BinIDValueSet&,float) const;
    void			setTextureData(int channel,
	    				       const BinIDValueSet*);
    const BinIDValueSet*	getCache(int channel) const;
    void			inValidateCache(int channel);

protected:
    				~HorizonSection();
    static ArrPtrMan<SbVec2f>	texturecoordptr_;				
    void			surfaceChangeCB(CallBacker*);
    void			removeZTransform();
    void			updateZAxisVOI(const Geometry::BinIDSurface*);

    void			updateTexture(int channel);
    void			updateAutoResolution(SoState*);
    static void			updateAutoResolution(void*,SoAction*);
    void			turnOnWireframe(int res);
    void			updateTileNeighbors(int nrrows,int nrcols);
    void			insertRowColTilesArray(bool row,bool bef,int);
    void			updateBBox(SoGetBoundingBoxAction* action);
    HorizonSectionTile*		createTile(int rowidx,int colidx);

    Geometry::BinIDSurface*	geometry_;
    Threads::Mutex		geometrylock_;
    ObjectSet<BinIDValueSet>	cache_;

    TextureChannels*		channels_;
    ColTabTextureChannel2RGBA*	channel2rgba_;
    SoTextureCoordinate2*	texturecrds_;

    SoCallback*			callbacker_;

    Array2DImpl<HorizonSectionTile*> tiles_;
    bool			usewireframe_;

    Transformation*		transformation_;
    ZAxisTransform*		zaxistransform_;
    int				zaxistransformvoi_; 
    				//-1 not needed by zaxistransform, -2 not set
				
    int				desiredresolution_;

    RowCol			origin_;
    RowCol			step_;
};

mClass HorizonSectionTile
{
public:
				HorizonSectionTile();
				~HorizonSectionTile();
    void			setResolution(int);
    				/*!<Resolution -1 means it is automatic. */
    void			updateAutoResolution(SoState*);
    				/*<Update only when the resolutionis -1. */
    void			setNeighbor(int,HorizonSectionTile*);
    void			setPos(int row,int col,const Coord3&);
    void			setDisplayTransformation(Transformation*);

    void			setMaxSpacing(int); 
				//Based on defined nrcells in the biginning
    int				getMaxSpacing() const	{ return maxspacing_; }
    				//Not used yet

    void			setTextureSize(int rowsz,int colsz);
    void			setTextureOrigin(int globrow,int globcol);

    void			setNormal(int idx,const Coord3& normal);
    int				getNormalIdx(int crdidx,int res) const;

    void			resetResolutionChangeFlag();

    void			tesselateActualResolution();
    void			updateGlue();

    void			useShading(bool);
    void			useWireframe(bool);
    void			turnOnWireframe(int res);
    void			setWireframeMaterial(Material*);
    void			setWireframeColor(Color col);
    
    SbBox3f			getBBox() const; 
    SoLockableSeparator*	getNodeRoot() const	{ return root_; }

protected:

    friend			class HorSectTileResolutionTesselator;
    int				getActualResolution() const;
    void			setActualResolution(int);
    int				getAutoResolution(SoState*);
    void			tesselateGlue();
    void			tesselateResolution(int);
    void			updateBBox();
    void			setWireframe(int res);

    bool			usewireframe_;
    Material*			wireframematerial_;
    bool			useshading_;

    HorizonSectionTile*		neighbors_[9];

    Coord3			bboxstart_;	//Display space
    Coord3			bboxstop_;	//Display space
    bool			needsupdatebbox_;

    SoLockableSeparator*	root_;
    visBase::Coordinates*	coords_;
    SoTextureComposer*		texture_;
    SoSwitch*			resswitch_;
    SoNormal*			normals_;
    int				maxspacing_;
    static int			normalstartidx_[mHorSectNrRes];
    static int			spacing_[mHorSectNrRes];
    static int			nrcells_[mHorSectNrRes];
    int				desiredresolution_;
    bool			resolutionhaschanged_;

    bool			needsretesselation_[mHorSectNrRes];
    SoGroup*			resolutions_[mHorSectNrRes];
    SoIndexedTriangleStripSet*	triangles_[mHorSectNrRes];
    SoIndexedLineSet*		lines_[mHorSectNrRes];
    SoIndexedLineSet*		wireframes_[mHorSectNrRes];
    SoDGBIndexedPointSet*	points_[mHorSectNrRes];
    SoSwitch*			wireframeswitch_[mHorSectNrRes];
    SoSeparator*		wireframeseparator_[mHorSectNrRes];
    Texture2*			wireframetexture_;

    visBase::Coordinates*	gluecoords_;
    SoIndexedTriangleStripSet*	gluetriangles_;
    SoSwitch*			gluelowdimswitch_;
    SoIndexedLineSet*		gluelines_;
    SoDGBIndexedPointSet*	gluepoints_;
    bool			glueneedsretesselation_;
};

};


#endif
