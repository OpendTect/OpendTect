#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uigeom.h"
#include "uiworld2ui.h"
#include "draw.h"

class uiRGBArray;

/*! \brief: Fills an uiRGBArray ( image buffer ) used to draw annotations 
  on 2D images

*/

typedef Geom::Point2D<double> dPoint;
typedef Geom::Point2D<int> iPoint;

mExpClass(uiTools) AnnotBufferFiller
{
public:

    mExpClass(uiTools) LineInfo
    {
	public:
				LineInfo()	{}
	virtual			~LineInfo()	{}

	OD::LineStyle		linestyle_;
	TypeSet<dPoint>		pts_;
    };
    //TODO add later on sth for the polygones

    
			AnnotBufferFiller(const uiWorld2Ui&);
			~AnnotBufferFiller();

    void		addLineInfo(const OD::LineStyle&,
				    const TypeSet<dPoint>&,
				    bool ishgrid,bool isvgrid);
    void		setW2UI(const uiWorld2Ui&);
    void		fillBuffer(const uiWorldRect&,uiRGBArray&) const;
    void		fillInterWithBufArea(const uiWorldRect&,const LineInfo&,
	    				     uiRGBArray&) const;

    void		eraseGridLines(bool);
    bool		dispannotlines_				= true;
    bool		disphorgdlines_				= true;
    bool		dispvertgdlines_			= true;

protected:

    uiWorld2Ui		w2u_;
    ObjectSet<LineInfo> annotlines_;
    ObjectSet<LineInfo> horgdlines_;
    ObjectSet<LineInfo> vertgdlines_;

    void		setPoint(const iPoint&,int,uiRGBArray&) const;
    void		setLine(const iPoint&,const iPoint&,
				const LineInfo&,uiRGBArray&) const;
    bool		isLineOutside(const LineInfo&,const uiWorldRect&) const;
    dPoint		computeIntersect(const dPoint&,const dPoint&,
					 const uiWorldRect&)const;
    void		dummytest();
};
