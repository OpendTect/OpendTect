#ifndef vishorizonsection_h
#define vishorizonsection_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: vishorizonsection.h,v 1.10 2009-04-28 19:39:43 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "arrayndimpl.h"
#include "position.h"
#include "visobject.h"

class BinIDValueSet;
class SbVec2f;
class SoAction;
class SoCallback;
class SoGroup;
class SoState;
class SoIndexedLineSet;
class SoIndexedPointSet;
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
class IndexedPolyLine;
class TextureChannels;
class TileResolutionTesselator;


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

    void			setSurface(Geometry::BinIDSurface*);
    Geometry::BinIDSurface*	getSurface() const	{ return geometry_; }

    void			useWireframe(bool);
    bool			usesWireframe() const;
    
    char			nrResolutions() const { return mHorSectNrRes; }
    char			currentResolution() const;
    void			setResolution(int);

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

    void			computeNormal(int r,int c,int spacing,Coord3&);
    void			setTileNormals(HorizonSectionTile&,
	    				       int startrow,int startcol);
protected:
    				~HorizonSection();
    static ArrPtrMan<SbVec2f>	texturecoordptr_;				
    void			updateGeometry();
    void			updateTexture(int channel);
    void			updateResolution(SoState*);
    static void			updateResolution(void*,SoAction*);
    void			updateWireFrame(int res);
    void			turnOnWireFrame(int res);

    Geometry::BinIDSurface*	geometry_;
    ObjectSet<BinIDValueSet>	cache_;

    TextureChannels*		channels_;
    ColTabTextureChannel2RGBA*	channel2rgba_;
    SoTextureCoordinate2*	texturecrds_;

    SoCallback*			callbacker_;

    Array2DImpl<HorizonSectionTile*> tiles_;
    visBase::IndexedPolyLine*	wireframelines_[mHorSectNrRes];

    Transformation*		transformation_;
    ZAxisTransform*		zaxistransform_;

    double			sinanglexinl_;
    double			cosanglexinl_;
    float			rowdistance_;
    float			coldistance_;    
    int				desiredresolution_;
};

mClass HorizonSectionTile
{
public:
				HorizonSectionTile();
				~HorizonSectionTile();
    void			setResolution(int);
    				/*!<Resolution -1 means it is automatic. */
    void			updateResolution(SoState*);
    				/*<Update only when the resolutionis -1. */
    void			setNeighbor(int,HorizonSectionTile*);
    void			setPos(int row,int col,const Coord3&);
    void			setDisplayTransformation(Transformation*);

//    void			setMaxSpacing();
    void			setTextureSize(int rowsz,int colsz);
    void			setTextureOrigion(int globrow,int globcol);

    void			setNormal(int idx,const Coord3& normal);
    int				getNormalIdx(int crdidx,int res) const;

    void			turnOffLines(bool);
    void			resetResolutionChangeFlag();

    void			tesselateActualResolution();
    void			updateGlue();
    SoLockableSeparator*	getNodeRoot() const	{ return root_; }

protected:

    int				getActualResolution() const;
    void			setActualResolution(int);
    int				getAutoResolution(SoState*);
    void			tesselateGlue();
    void			tesselateResolution(int);
    void			updateBBox();

    HorizonSectionTile*		neighbors_[9];

    Coord3			bboxstart_;	//Display space
    Coord3			bboxstop_;	//Display space
    bool			needsupdatebbox_;

    SoLockableSeparator*	root_;
    visBase::Coordinates*	coords_;
    SoTextureComposer*		texture_;
    SoSwitch*			resswitch_;
    SoNormal*			normals_;
    int				normalstartidx[mHorSectNrRes];
    static int			spacing_[mHorSectNrRes];
    static int			nrcells_[mHorSectNrRes];
    int				desiredresolution_;
    bool			resolutionhaschanged_;

    bool			needsretesselation_[mHorSectNrRes];
    SoGroup*			resolutions_[mHorSectNrRes];
    SoIndexedTriangleStripSet*	triangles_[mHorSectNrRes];
    SoIndexedLineSet*		lines_[mHorSectNrRes];
    SoIndexedPointSet*		points_[mHorSectNrRes];

    visBase::Coordinates*	gluecoords_;
    SoIndexedTriangleStripSet*	gluetriangles_;
    SoIndexedLineSet*		gluelines_;
    SoIndexedPointSet*		gluepoints_;
    bool			glueneedsretesselation_;
};

};


#endif
