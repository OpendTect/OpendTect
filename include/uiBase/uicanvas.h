#ifndef uiCanvas_H
#define uiCanvas_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.h,v 1.8 2004-10-29 10:27:21 nanne Exp $
________________________________________________________________________

-*/
#include <uidrawable.h>
#include <uimouse.h>

class	uiMouseEvent;
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

    void			setRubberBandingOn(uiMouseEvent::ButtonState);
    uiMouseEvent::ButtonState	rubberBandingOn() const;
    void			setAspectRatio( float );
    float			aspectRatio();

    CNotifier<uiScrollView,const uiMouseEvent&>	mousepressed;
    CNotifier<uiScrollView,const uiMouseEvent&>	mousemoved;
    CNotifier<uiScrollView,const uiMouseEvent&>	mousereleased;
    CNotifier<uiScrollView,const uiMouseEvent&>	mousedoubleclicked;

private:

    uiScrollViewBody*		body_;
    uiScrollViewBody&		mkbody(uiParent*,const char*);


};

#endif
