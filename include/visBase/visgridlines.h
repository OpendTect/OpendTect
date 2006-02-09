#ifndef visgridlines_h
#define visgridlines_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		December 2005
 RCS:		$Id: visgridlines.h,v 1.2 2006-02-09 07:48:06 cvshelene Exp $
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

    void			setDisplayTransformation(Transformation*);

    void			setLineStyle(const LineStyle&);
    void			getLineStyle(LineStyle&) const;

    void			setCubeSampling(const CubeSampling&);
    const CubeSampling&		getCubeSampling() 		{ return cs_; }

    void			showInlines(bool);
    bool			areInlinesShown() const;
    void			showCrosslines(bool);
    bool			areCrosslinesShown() const;
    void			showZlines(bool);
    bool			areZlinesShown() const;

protected:

    CubeSampling		cs_;
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

    IndexedPolyLine*		addLineSet();
    void			addLine(IndexedPolyLine&,const Coord3& start,
					const Coord3& stop);

    void			drawInlines();
    void			drawCrosslines();
    void			drawZlines();
};


}; // Namespace visBase

#endif
