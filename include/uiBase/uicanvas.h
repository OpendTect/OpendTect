#ifndef uiCanvas_H
#define uiCanvas_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uicanvas.h,v 1.1 2000-11-27 10:19:27 bert Exp $
________________________________________________________________________

-*/
#include <uidrawable.h>
#include <uimouse.h>

class i_QCanvas;
class i_ScrollableCanvas;
class uiMouseEvent;

class uiCanvasABC : public uiDrawableObj
{
    friend class 	i_QCanvas;
    friend class 	i_ScrollableCanvas;

public:
                        uiCanvasABC(uiObject*,const char *nm="uiCanvasABC");
    virtual             ~uiCanvasABC() {};

    static int          getDefaultWidth()          { return defaultWidth; }
    static void         setDefaultWidth( int w )   { defaultWidth = w; }

    static int          getDefaultHeight()         { return defaultHeight; }
    static void         setDefaultHeight( int h )  { defaultHeight = h; }

protected:

    static int          defaultWidth;
    static int          defaultHeight;

};

class uiCanvas : public uiMltplWrapObj<uiCanvasABC,i_QCanvas>
{
public:

			uiCanvas( uiObject* parnt=0, const char* nm="uiCanvas");
    virtual		~uiCanvas();


protected:

    virtual const QWidget*  qWidget_() const;
};

class uiScrollView : public uiMltplWrapObj<uiCanvasABC,i_ScrollableCanvas>
{
friend class i_ScrollableCanvas;
public:

                        uiScrollView( uiObject* parnt,
                                      const char *nm = "uiScrollView" );

    virtual             ~uiScrollView();

    virtual QPaintDevice* mQPaintDevice();

    void                resizeContents ( int w, int h );
    inline void		resizeContents ( uiSize s ) 
                        { resizeContents( s.width(), s.height() ); }
    void                setContentsPos ( uiPoint topLeft );
    void                updateContents();
    void        	updateContents( uiRect area, bool erase = true );
    uiRect		visibleArea() const;

    int 		frameWidth() const;
    virtual void	setPrefWidth( int w )      
			{ 
			    pref_char_width = -1;
			    pref_width = w + 2*frameWidth(); 
			}
    virtual void	setPrefHeight( int h )     
			{ 
			    pref_char_height = -1;
			    pref_height = h + 2*frameWidth(); 
			}

    virtual void        rubberBandHandler(uiRect) {}
    void		setRubberBandingOn(uiMouseEvent::ButtonState);
    uiMouseEvent::ButtonState rubberBandingOn() const;
    void		setAspectRatio( float );
    float		aspectRatio();

    CNotifier<uiScrollView,const uiMouseEvent&>	mousepressed;
    CNotifier<uiScrollView,const uiMouseEvent&>	mousemoved;
    CNotifier<uiScrollView,const uiMouseEvent&>	mousereleased;
    CNotifier<uiScrollView,const uiMouseEvent&>	mousedoubleclicked;

protected:

    virtual const QWidget*  qWidget_() const;

    virtual void	contentsMousePressHandler( const uiMouseEvent& e )
			{ mousepressed.trigger(e); }
    virtual void	contentsMouseMoveHandler( const uiMouseEvent& e )
			{ mousemoved.trigger(e); }
    virtual void	contentsMouseReleaseHandler( const uiMouseEvent& e )
			{ mousereleased.trigger(e); }
    virtual void	contentsMouseDoubleClickHandler( const uiMouseEvent& e )
			{ mousedoubleclicked.trigger(e); }

    virtual void        forceRedraw_( bool deep )
			{ uiObject::forceRedraw_( deep ); updateContents();}
};

#endif
