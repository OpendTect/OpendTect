#ifndef visgridlines_h
#define visgridlines_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		December 2005
 RCS:		$Id: visgridlines.h,v 1.11 2011/12/16 15:57:20 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "cubesampling.h"

class IOPar;
class LineStyle;

namespace visBase
{

/*!\brief
*/

class DrawStyle;
class IndexedPolyLine;
class Transformation;

mClass GridLines : public VisualObjectImpl
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

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int			usePar(const IOPar&);

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
    const mVisTrans*		transformation_;

    void			emptyLineSet(IndexedPolyLine*);
    IndexedPolyLine*		addLineSet();
    void			addLine(IndexedPolyLine&,const Coord3& start,
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
