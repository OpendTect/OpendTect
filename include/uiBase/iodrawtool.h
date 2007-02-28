#ifndef iodrawtool_h
#define iodrawtool_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          04/07/2001
 RCS:           $Id: iodrawtool.h,v 1.17 2007-02-28 21:15:44 cvskris Exp $
________________________________________________________________________

-*/

#include "iodraw.h"
#include "uigeom.h"
#include "sets.h"
#include "color.h"

class QPaintDevice;
class QPainter;
class QPen;

class ioPixmap;
class uiFont;
class Color;
class Alignment;
class LineStyle;
class MarkerStyle2D;

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

    void	drawLine( int x1, int y1, int x2, int y2 );
    void	drawLine( const uiPoint& p1, const uiPoint& p2 );
    void	drawLine(const TypeSet<uiPoint>&,int idx1=0,int nr=-1);
    		//<!Draws a line defined by 'nr' points starting at idx1
    
    void	drawPolygon(const TypeSet<uiPoint>&,int i1=0,int nr=-1);
    		/*<!Draws a polygon defined by 'nr' points starting at i1*/

    void	drawText( int x, int y, const char *, const Alignment&, 
			  bool over=true, bool erase=false);
    void	drawText( const uiPoint& p,const char * txt,
	    		  const Alignment& al, bool over=true,bool erase=false);

    void 	drawRect( int x, int y, int w, int h ); 
    		/*!<\note that w and h are the outer width of the rectangle. */
    void	drawRect( const uiPoint& topLeft, const uiSize& sz );
    		/*!<\note that sz is the outer width of the rectangle.*/
    void	drawRect( const uiRect& r );

    void 	drawEllipse( int x, int y, int w, int h ); 
    void	drawEllipse( const uiPoint&, const uiSize& );
    void 	drawEllipse( const uiRect& );

    void	drawBackgroundPixmap(const Color* c=0);

    void 	drawPixmap( const uiPoint& destTopLeft, const ioPixmap*, 
			    const uiRect& srcAreaInPixmap );
    void	drawPixmap( int left, int top, ioPixmap* pm , 
			    int sLeft, int sTop, int sRight, int sBottom );

    void	drawMarker(const uiPoint&,const MarkerStyle2D&,
	    		   const char* txt=0,bool below=true);

    uiSize	getDevSize() const;
    int 	getDevHeight() const;
    int 	getDevWidth() const;

    void	setLineStyle( const LineStyle& );
    void	setPenColor( const Color& );
    void	setFillColor( const Color& );
    void	setPenWidth( unsigned int );

    void	setFont( const uiFont& f);
    const uiFont* font()			{ return font_; }

    void	setOrigin( const uiPoint& tl )	{ setOrigin( tl.x, tl.y ); }
    void	setOrigin( int x_0, int y_0 )	{ x0_ = x_0; y0_ = y_0; }

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
