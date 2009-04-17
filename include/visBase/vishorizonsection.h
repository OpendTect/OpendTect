#ifndef vishorizonsection_h
#define vishorizonsection_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: vishorizonsection.h,v 1.8 2009-04-17 20:03:10 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "arrayndimpl.h"

class BinIDValueSet;
class SoAction;
class SoCallback;
class SoGroup;
class SoState;
class SoIndexedLineSet;
class SoIndexedPointSet;
class SoIndexedTriangleStripSet;
class SoNormal;
class SoTextureComposer;
class ZAxisTransform;

namespace Geometry { class BinIDSurface; }


#define mHorSectNrRes		6
#define mHorSectSideSize	62 //This is the size of blocks of the tile
#define mHorSectNrSideKnots	63 

namespace visBase
{

class ColTabTextureChannel2RGBA;    
class Coordinates;
class HorizonSectionTile;
class PolyLine;
class TextureChannels;
class TileResolutionTesselator;


mClass HorizonSection : public VisualObjectImpl
{
public:
    static HorizonSection*	create() mCreateDataObj(HorizonSection);

    void			setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();
    void			setZAxisTransform(ZAxisTransform*);

    void			setGeometry(Geometry::BinIDSurface*);
    Geometry::BinIDSurface*	getGeometry() const	{ return geometry_; }

    void			useWireframe(bool);
    bool			usesWireframe() const;
    
    char			nrResolutions() const;
    char			currentResolution() const;
    void			setResolution(char);

    void			getDataPositions(BinIDValueSet&,float) const;
    void			setDisplayData(const BinIDValueSet*);

    void			replaceChannels(visBase::TextureChannels*);
    TextureChannels*		getTextureChannels() const { return channels_; }

    void			computeNormal(int r,int c,int spacing,Coord3&);
    void			setTileNormals(HorizonSectionTile&,
	    				       int startrow,int startcol);
    				/*<Start row/col is the tile origin. */
    
protected:
    				~HorizonSection();
    friend			class HorizonTileCreater;			
    void			updateGeometry();
    void			updateResolution(SoState*);
    static void			updateResolution(void*,SoAction*);

    Geometry::BinIDSurface*	geometry_;

    TextureChannels*		channels_;
    ColTabTextureChannel2RGBA*	channel2rgba_;

    SoCallback*			callbacker_;

    Array2DImpl<HorizonSectionTile*> tiles_;
    visBase::PolyLine*		wireframelines_;

    Transformation*		transformation_;
    ZAxisTransform*		zaxistransform_;

    double			sinanglexinl_;
    double			cosanglexinl_;
    float			rowdistance_;
    float			coldistance_;    
};

mClass HorizonSectionTile
{
public:
				HorizonSectionTile();
				~HorizonSectionTile();
    friend			class TileResolutionTesselator;    
    void			setResolution(int);
    				/*!<Resolution -1 means it will be vary, it will				    be set when we call updateResolution. */
    int				getActualResolution() const;
    void			setActualResolution(int);
    void			updateResolution(SoState*);
    				/*<Update only when the resolutionis -1. */
    void			setNeighbor(int,HorizonSectionTile*);
    void			setPos(int row,int col,const Coord3&);

    void			setTextureComposerSize(int,int);
    void			setTextureComposerOrig(int globrow,int globcol);

    void			setNormal(int idx,const Coord3& normal);
    int				getNormalIdx(int crdidx,int res) const;

    void			updateGlue();
    SoLockableSeparator*	getNodeRoot() const	{ return root_; }
    visBase::Coordinates*	getCoords() const	{ return coords_; }

protected:

    int				getAutoResolution(SoState*);
    void			tesselateGlue();
    void			tesselateResolution(int);

    HorizonSectionTile*		neighbors_[9];

    SoLockableSeparator*	root_;
    visBase::Coordinates*	coords_;
    SoTextureComposer*		texture_;
    SoSwitch*			resswitch_;
    SoNormal*			normals_;
    int				normalstartidx[mHorSectNrRes];
    int				spacing[mHorSectNrRes];

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
