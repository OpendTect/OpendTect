#ifndef annotbuffill_h
#define annotbuffill_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          04/09/2006
 RCS:           $Id: annotbuffill.h,v 1.2 2006-09-07 15:15:24 cvshelene Exp $
________________________________________________________________________

-*/

#include "uigeom.h"
#include "draw.h"

class uiWorld2Ui;
class ArrayRGB;

/*! \brief: Fills an ArrayRGB ( image buffer ) used to draw annotations 
  on 2D images

*/

typedef Geom::Point2D<double> dPoint;
typedef Geom::Point2D<int> iPoint;

class AnnotBufferFiller
{

public:

    class LineInfo
    {
	public:

	    LineStyle               		linestyle_;
	    TypeSet<dPoint>      		pts_;
    };
    //TODO add later on sth for the polygones

    
			AnnotBufferFiller(const uiWorld2Ui* w=0)
			: w2u_(w)			{}

			~AnnotBufferFiller()		{ deepErase(lines_); }

    void		setW2UI( const uiWorld2Ui* w )	{ w2u_ = w; }
    void		fillBuffer(const uiWorldRect&,ArrayRGB&) const;
    void		fillInterWithBufArea(const uiWorldRect&,int,
	    				     ArrayRGB&) const;


protected:

    const uiWorld2Ui*	w2u_;
    ObjectSet<LineInfo> lines_;

    void		setPoint(const iPoint&,int,ArrayRGB&) const;
    void		setLine(const iPoint&,const iPoint&,
	    			int,ArrayRGB&) const;
    bool		isLineOutside(const LineInfo*,const uiWorldRect&) const;
    dPoint		computeIntersect(const dPoint&,const dPoint&,
	    				 const uiWorldRect&)const;
    void		dummytest();
};


#endif
