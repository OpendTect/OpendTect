#ifndef vishorizonsection_h
#define vishorizonsection_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id: vishorizonsection.h,v 1.1 2009-03-03 19:13:39 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"

#define mHorizonSectionNrRes	6
#define mHorizonSectionSideSize	62
namespace visBase
{
class PolyLine;
class HorizonSectionPart;

mClass HorizonSection : public VisualObjectImpl
{
public:
    HorizonSection*		create()
				mCreateDataObj(HorizonSection*);

    void			setDisplayTransformation(Transformation*);
    Transformation*		getDisplayTransformation();
    
    void			setZAxisTransform(ZAxisTransform*);

    void			setGeometry(Geometry::BinIDSurface*);

protected:

    void			geomChangeCB(CallBacker*);
    void			updateResolutions(CallBacker*);

    Geometry::BinIDSurface*	geometry_;

    Array2D<HorizonSectionTile>	tiles_;
    visBase::PolyLine*		wireframelines_;
};

class HorizonSectionTile
{
public:
		HorizonSectionTile();
		~HorizonSectionTile();
    void	setResolution(int);
    		//!<Sets the resolution. -1 means resolution will vary
    int		getActualResolution() const;
    void	updateResolution(SoState*);
    void	setNeighbor(int,HorizonSectionPart*);
    void	setPos(int row,int col,const Coord3&);

    void	updateGlue();

protected:
    void	tesselateGlue();
    void	tesselateResolution(int);

    HorizonSectionPart*		neighbors_[9];
    int				neighborresolutions_[9];

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
