#ifndef uiCanvas_H
#define uiCanvas_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.h,v 1.3 2001-08-23 14:59:17 windev Exp $
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


    void			resizeContents( int w, int h );
    inline void			resizeContents( uiSize s ) 
				    { resizeContents( s.width(), s.height() ); }
    void			setContentsPos( uiPoint topLeft );

    void			updateContents();
    void			updateContents( uiRect area, bool erase= true );
    uiRect			visibleArea() const;

    int				frameWidth() const;
    void			setPrefContentsWidth( int w )      
				    { setPrefWidth( w + 2*frameWidth() ); }
    void			setPrefContentsHeight( int h )     
				    { setPrefHeight( h + 2*frameWidth() ); }

    virtual void		rubberBandHandler(uiRect)	{}

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
