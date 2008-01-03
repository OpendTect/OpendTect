#ifndef iodrawtool_h
#define iodrawtool_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawtool.h,v 1.27 2008-01-03 12:24:22 cvsnanne Exp $
________________________________________________________________________

-*/

#include "iodraw.h"
#include "uigeom.h"
#include "draw.h"

class Color;
class uiFont;
class ioPixmap;
class QPainter;
class QPaintDevice;
class QPen;


/*!\brief Tool to draw on ioDrawAreas.

  You should not try to construct one yourself; rather, get one from an
  ioDrawArea.
 */

class ioDrawTool
{
public:

    Color		backgroundColor() const;
    void		setBackgroundColor(const Color&);
    void		clear(const uiRect* r=0,const Color* c=0);

    void		setLineStyle(const LineStyle&);
    void		setPenColor(const Color&);
    void		setFillColor(const Color&);
    void		setPenWidth(unsigned int);
    void		setFont(const uiFont&);
    const uiFont*	font()			{ return font_; }

    void		drawText(int x,int y,const char*,const Alignment&);
    void		drawText(const uiPoint& p,const char*,const Alignment&);
    			/*! Alignment = Start, Middle or Stop (see draw.h)
			    Example: mAlign(Middle,Stop) */

    void		drawLine(int x1,int y1,int x2,int y2);
    void		drawLine(const uiPoint&,const uiPoint&);
    void		drawLine(const uiPoint&,double angle,double len);
    void		drawLine(const TypeSet<uiPoint>&,bool close);
    void 		drawRect(int x,int y,int w,int h); 
    void		drawRect(const uiPoint& topLeft,const uiSize& sz);
    void		drawRect(const uiRect&);
    inline void		drawPolyline( const TypeSet<uiPoint>& pts )
			{ drawLine( pts, false ); }
    inline void		drawPolygon( const TypeSet<uiPoint>& pts )
			{ drawLine( pts, true ); }

    void 		drawEllipse(int x,int y,int rx,int ry); 
    void		drawEllipse(const uiPoint&,const uiSize&);
    inline void		drawCircle( int x, int y, int r )
       			{ drawEllipse( x, y, r, r ); }	
    inline void		drawCircle( const uiPoint& p, int r )
       			{ drawEllipse( p, uiSize(r,r) ); }	
    void		drawHalfSquare(const uiPoint& from,double angle,
	    			       double diameter,bool left);
    void		drawHalfSquare(const uiPoint& from,const uiPoint& to,
	    			       bool left);

    void		drawPoint(const uiPoint&,bool highlight=false);
    			//!< Draws 3x3, h/v cross, square if highlight
    void		drawMarker(const uiPoint&,const MarkerStyle2D&,
	    			   float angle=0,int side=0);
    			//!< side -1=left half, 1=right half
    void		drawArrow(const uiPoint& tail,const uiPoint& head,
				  const ArrowStyle&);

    void		drawBackgroundPixmap(const Color* c=0);
    void 		drawPixmap(const uiPoint& destTopLeft,const ioPixmap*, 
				   const uiRect& srcAreaInPixmap);
    void		drawPixmap(int left,int top,ioPixmap*, 
				   int srcleft,int srctop,
				   int srcright,int srcbot);

    void		setRasterXor();
    void		setRasterNorm();

    int 		getDevHeight() const;
    int 		getDevWidth() const;

    virtual		~ioDrawTool(); 

protected:

    friend class ioDrawAreaImpl;
			ioDrawTool(QPaintDevice*);

    QPainter*		qpainter_;
    bool		qpaintermine_;
    bool		qpainterprepared_;

    void		preparePainter() const;

public:

    void		setActivePainter(QPainter*);
    void		dismissPainter();
    QPainter*		qPainter()		{ return qpainter_; }
    QPen&		qPen()			{ return qpen_; }
    QPaintDevice&	qPaintDevice()		{ return qpaintdev_; }

private:

    QPen&		qpen_;
    QPaintDevice&	qpaintdev_;

    const uiFont*	font_;
};

#endif
