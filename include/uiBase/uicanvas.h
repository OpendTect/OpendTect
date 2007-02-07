#ifndef uicanvas_h
#define uicanvas_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.h,v 1.13 2007-02-07 14:10:15 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidrawable.h"
#include "mouseevent.h"

class	uiCanvasBody;
class	uiScrollViewBody;

class uiCanvasDefaults
{
friend class			uiCanvasBody;
friend class			uiScrollViewBody;

public:

    static int			getDefaultWidth()	{ return defaultWidth; }
    static void			setDefaultWidth(int w)	{ defaultWidth = w; }

    static int			getDefaultHeight()	{ return defaultHeight;}
    static void			setDefaultHeight(int h)	{ defaultHeight = h; }

protected:

    static int			defaultWidth;
    static int			defaultHeight;
};

class uiCanvas : public uiDrawableObj
{
public:
				uiCanvas(uiParent*,const char *nm="uiCanvas");
    virtual			~uiCanvas()			{}

    void			update();

private:

    uiCanvasBody*		body_;
    uiCanvasBody&		mkbody(uiParent*,const char*);

};


class uiScrollView : public uiDrawableObj
{
public:

				uiScrollView( uiParent* parnt,
					      const char *nm = "uiScrollView" );

    virtual			~uiScrollView()			{}

    void			update();

    enum ScrollBarMode		{ Auto, AlwaysOff, AlwaysOn };
    void			setScrollBarMode(ScrollBarMode,bool hor);
    ScrollBarMode		getScrollBarMode(bool hor) const;

    void			resizeContents( int w, int h );
    inline void			resizeContents( uiSize s ) 
				    { resizeContents(s.hNrPics(),s.vNrPics()); }
    void			setContentsPos( uiPoint topLeft );

    void			updateContents();
    void			updateContents( uiRect area, bool erase= true );
    uiRect			visibleArea() const;

    int				frameWidth() const;

    virtual void		rubberBandHandler(uiRect)	{}

    virtual void		setPrefWidth( int w );
    virtual void		setPrefHeight( int h );
    virtual void		setMaximumWidth( int w );
    virtual void		setMaximumHeight( int h );

    void			setRubberBandingOn(OD::ButtonState);
    OD::ButtonState		rubberBandingOn() const;
    void			setAspectRatio( float );
    float			aspectRatio();
 
    				// revieve mouse events w/o pressing button
    void			setMouseTracking(bool yn = true);

    MouseEventHandler&		getMouseEventHandler(); 

private:
    MouseEventHandler		mousehandler_;

    uiScrollViewBody*		body_;
    uiScrollViewBody&		mkbody(uiParent*,const char*);


};

#endif
