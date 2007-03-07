#ifndef iodrawtool_h
#define iodrawtool_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawtool.h,v 1.18 2007-03-07 10:34:38 cvsbert Exp $
________________________________________________________________________

-*/

#include "iodraw.h"
#include "uigeom.h"
#include "sets.h"
#include "color.h"

class ioPixmap;
class uiFont;
class Color;
class Alignment;
class LineStyle;
class MarkerStyle2D;
class QPaintDevice;
class QPainter;
class QPen;

#ifdef USEQT3
class QPaintDeviceMetrics; 
#endif


//! Tool to draw on ioDrawArea's. Each ioDrawArea can give you a drawtool.
class ioDrawTool
{   

    friend class	ioDrawAreaImpl;
    friend class	uiScrollViewBody;

mProtected:
		ioDrawTool(QPaintDevice*,int x_0=0,int y_0=0);
public:

    virtual	~ioDrawTool(); 

    Color	backgroundColor() const;
    void	setBackgroundColor(const Color&);
    void	clear(const uiRect* r=0,const Color* c=0);

    void	drawText(int x,int y,const char*,const Alignment&);
    void	drawText(const uiPoint& p,const char*,const Alignment&);

    void	drawLine(int x1,int y1,int x2,int y2);
    void	drawLine(const uiPoint&,const uiPoint&);
    void	drawLine(const TypeSet<uiPoint>&,int idx1=0,int nr=-1);
    		//<!Draws a line defined by 'nr' points starting at idx1
    void	drawPolygon(const TypeSet<uiPoint>&,int i1=0,int nr=-1);
    		/*<!Draws a polygon defined by 'nr' points starting at i1*/
    		/*! Alignment = Start, Middle or Stop (see draw.h)
		    Example: Alignment(Alignment::Middle,Alignment::Stop) */

    void 	drawRect(int x,int y,int w,int h); 
    void	drawRect(const uiPoint& topLeft,const uiSize& sz);
    void	drawRect(const uiRect&);
    void 	drawEllipse(int x,int y,int w,int h); 
    void	drawEllipse(const uiPoint&,const uiSize&);
    void 	drawEllipse(const uiRect&);

    void	drawMarker(const uiPoint&,const MarkerStyle2D&,
	    		   const char* txt=0,bool below=true);

    void	drawBackgroundPixmap(const Color* c=0);
    void 	drawPixmap(const uiPoint& destTopLeft,const ioPixmap*, 
			   const uiRect& srcAreaInPixmap);
    void	drawPixmap(int left,int top,ioPixmap*, 
			   int srcleft,int srctop,int srcright,int srcbot);

    uiSize	getDevSize() const;
    int 	getDevHeight() const;
    int 	getDevWidth() const;

    void	setLineStyle(const LineStyle&);
    void	setPenColor(const Color&);
    void	setFillColor(const Color&);
    void	setPenWidth(unsigned int);

    void	setFont(const uiFont&);
    const uiFont* font()			{ return font_; }

    void	setOrigin( const uiPoint& tl )	{ setOrigin( tl.x, tl.y ); }
    void	setOrigin( int x, int y )	{ x0_ = x; y0_ = y; }

    bool 	active() const			{ return active_; }
    bool	beginDraw(); 
    bool	endDraw();

    void	setRasterXor();
    void	setRasterNorm();

protected:

    bool	setActivePainter(QPainter*);
    QPainter*	qpainter_;

private:

    QPen&			qpen_;
    bool			freeqpainter_;
    QPaintDevice*		qpaintdev_;
#ifdef USEQT3
    QPaintDeviceMetrics*	qpaintdevmetr_;
#endif
    bool			active_;
    int				x0_;
    int				y0_;
    const uiFont*		font_;

};

#endif
