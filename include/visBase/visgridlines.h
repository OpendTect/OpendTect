#ifndef visgridlines_h
#define visgridlines_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		December 2005
 RCS:		$Id: visgridlines.h,v 1.5 2006-03-01 12:39:52 cvsnanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "cubesampling.h"

class LineStyle;

namespace visBase
{

/*!\brief
*/

class DrawStyle;
class IndexedPolyLine;
class Transformation;

class GridLines : public VisualObjectImpl
{
public:
    static GridLines*		create()
    				mCreateDataObj(GridLines);
    				~GridLines();

    void			setDisplayTransformation(Transformation*);

    void			setLineStyle(const LineStyle&);
    void			getLineStyle(LineStyle&) const;

    void			setGridCubeSampling(const CubeSampling&);
    void			setPlaneCubeSampling(const CubeSampling&);
    const CubeSampling&		getGridCubeSampling() 	{ return gridcs_; }
    const CubeSampling&		getPlaneCubeSampling() 	{ return planecs_; }

    void			showInlines(bool);
    bool			areInlinesShown() const;
    void			showCrosslines(bool);
    bool			areCrosslinesShown() const;
    void			showZlines(bool);
    bool			areZlinesShown() const;

protected:

    CubeSampling		gridcs_;
    CubeSampling		planecs_;
    bool			csinlchanged_;
    bool			cscrlchanged_;
    bool			cszchanged_;

    IndexedPolyLine*		inlines_;
    IndexedPolyLine*		crosslines_;
    IndexedPolyLine*		zlines_;
    IndexedPolyLine*		trcnrlines_;

    ObjectSet<IndexedPolyLine>	polylineset_;
    DrawStyle*			drawstyle_;
    Transformation*		transformation_;

    void			emptyLineSet(IndexedPolyLine*);
    IndexedPolyLine*		addLineSet();
    void			addLine(IndexedPolyLine&,const Coord3& start,
					const Coord3& stop);

    void			drawInlines();
    void			drawCrosslines();
    void			drawZlines();
};


}; // Namespace visBase

#endif
