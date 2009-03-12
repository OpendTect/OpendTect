#ifndef vishorizonsection_h
#define vishorizonsection_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: vishorizonsection.h,v 1.2 2009-03-12 20:41:53 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "arrayndimpl.h"

class SoState;
class SoCoordinate3;
class SoIndexedLineSet;
class SoIndexedTriangleStripSet;
class SoTextureComposer;
class ZAxisTransform;

namespace Geometry { class BinIDSurface; }


#define mHorizonSectionNrRes	6
#define mHorizonSectionSideSize	62

namespace visBase
{
class PolyLine;
class HorizonSectionTile;

mClass HorizonSection : public VisualObjectImpl
{
public:
    static HorizonSection*	create() mCreateDataObj(HorizonSection);

    void			setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();
    void			setZAxisTransform(ZAxisTransform*);

    void			setGeometry(Geometry::BinIDSurface*);

    void			useWireframe(bool);
    bool			usesWireframe() const;
    
    char			nrResolutions() const;
    char			currentResolution() const;
    void			setResolution(char);

protected:
    				~HorizonSection();
    void			geomChangeCB(CallBacker*);
    void			updateResolutions(CallBacker*);

    Geometry::BinIDSurface*	geometry_;

    Array2DImpl<HorizonSectionTile*> tiles_;
    visBase::PolyLine*		wireframelines_;

    Transformation*		transformation_;
    ZAxisTransform*		zaxistransform_;
};

class HorizonSectionTile
{
public:
				HorizonSectionTile();
				~HorizonSectionTile();
    void			setResolution(int);
    				//!<Resolution -1 means it will vary
    int				getActualResolution() const;
    void			updateResolution(SoState*);
    void			setNeighbor(int,HorizonSectionTile*);
    void			setPos(int row,int col,const Coord3&);

    void			updateGlue();

protected:

    void			tesselateGlue();
    void			tesselateResolution(int);

    HorizonSectionTile*		neighbors_[9];
    int				neighborresolutions_[9];

    int				resolution_;

    SoLockableSeparator*	root_;
    SoCoordinate3*		coords_;
    SoTextureComposer*		texture_;
    SoSwitch*			resswitch_;

    bool			needsretesselation_[mHorizonSectionNrRes];
    SoSeparator*		resolutions_[mHorizonSectionNrRes];
    SoIndexedTriangleStripSet*	triangles_[mHorizonSectionNrRes];
    SoIndexedLineSet*		lines_[mHorizonSectionNrRes];

    SoIndexedTriangleStripSet*	gluetriangles_;
    SoIndexedLineSet*		gluelines_;
    bool			glueneedsretesselation_;
};

};


#endif
