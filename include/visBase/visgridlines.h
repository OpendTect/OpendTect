#ifndef visgridlines_h
#define visgridlines_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		December 2005
 RCS:		$Id$
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visobject.h"
#include "cubesampling.h"

class LineStyle;

namespace visBase
{

/*!\brief
*/

class DrawStyle;
class PolyLine;
class Transformation;

mExpClass(visBase) GridLines : public VisualObjectImpl
{
public:
    static GridLines*		create()
    				mCreateDataObj(GridLines);
    				~GridLines();

    void			setDisplayTransformation(const mVisTrans*);

    void			setLineStyle(const LineStyle&);
    void			getLineStyle(LineStyle&) const;

    void			adjustGridCS();
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

    PolyLine*			inlines_;
    PolyLine*			crosslines_;
    PolyLine*			zlines_;
    PolyLine*			trcnrlines_;

    ObjectSet<PolyLine>		polylineset_;
    DrawStyle*			drawstyle_;
    const  mVisTrans*		transformation_;
    Material*			linematerial_;

    void			emptyLineSet(PolyLine*);
    PolyLine*			addLineSet();
    void			addLine(PolyLine&,const Coord3& start,
					const Coord3& stop);

    void			drawInlines();
    void			drawCrosslines();
    void			drawZlines();

    static const char*		sKeyLineStyle();
    static const char*		sKeyInlShown();
    static const char*		sKeyCrlShown();
    static const char*		sKeyZShown();
};


} // Namespace visBase

#endif

