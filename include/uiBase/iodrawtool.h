#ifndef iodrawtool_h
#define iodrawtool_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawtool.h,v 1.35 2012-08-23 11:19:10 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "iodraw.h"
#include "uigeom.h"
#include "draw.h"

class uiFont;
class ioPixmap;
mFDQtclass(QPainter)
mFDQtclass(QPaintDevice)
mFDQtclass(QPen)


/*!\brief Tool to draw on ioDrawAreas.

  You should not try to construct one yourself; rather, get one from an
  ioDrawArea.
 */

mClass(uiBase) ioDrawTool
{
public:

    enum BackgroundMode	{ Transparent, Opaque };

    void		setDrawAreaBackgroundColor(const Color&);
    void		setBackgroundMode(BackgroundMode);
    BackgroundMode	backgroundMode() const;
    Color		backgroundColor() const;
    void		setBackgroundColor(const Color&);
    void		clear(const uiRect* r=0,const Color* c=0);
    void		translate(float dx,float dy);
    void		rotate(float angle);

    void		setLineStyle(const LineStyle&);
    void		setPenColor(const Color&);
    void		setFillColor(const Color&);
    void		setPenWidth(unsigned int);
    void		setFont(const uiFont&);
    LineStyle		lineStyle() const;
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

    void 		drawPixmap(const uiPoint& destTopLeft,const ioPixmap*, 
				   const uiRect& srcAreaInPixmap);
    void		drawPixmap(int left,int top,ioPixmap*, 
				   int srcleft,int srctop,
				   int srcright,int srcbot);

    void		useBackgroundPattern( bool yn )
			{ usebgpattern_ = yn; }

    void		setRasterXor();
    void		setRasterNorm();

    int 		getDevHeight() const;
    int 		getDevWidth() const;

    virtual		~ioDrawTool(); 

protected:

    friend class ioDrawAreaImpl;
			ioDrawTool(mQtclass(QPaintDevice*));

    mQtclass(QPainter*)	qpainter_;
    bool		qpaintermine_;
    bool		qpainterprepared_;

    void		preparePainter() const;

public:

    void		setActivePainter(mQtclass(QPainter*));
    void		dismissPainter();
    mQtclass(QPainter*)	qPainter()		{ return qpainter_; }
    mQtclass(QPen&)	qPen()			{ return qpen_; }
    mQtclass(QPaintDevice&)	qPaintDevice()	{ return qpaintdev_; }

private:

    mQtclass(QPen&)	qpen_;
    mQtclass(QPaintDevice&)	qpaintdev_;

    const uiFont*	font_;
    Color		areabgcolor_;
    bool		usebgpattern_;
};

#endif

