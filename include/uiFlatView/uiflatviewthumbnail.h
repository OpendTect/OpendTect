#ifndef uiflatviewthumbnail_h
#define uiflatviewthumbnail_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Aug 2007
 RCS:           $Id: uiflatviewthumbnail.h,v 1.2 2007-11-07 16:54:46 cvskris Exp $
________________________________________________________________________

-*/

#include "uicanvas.h"
class uiFlatViewer;
class MouseEventHandler;


/*!\brief Shows a thumbnail with current position of a uiFlatViewer. */

class uiFlatViewThumbnail : public uiCanvas
{
public:
    			uiFlatViewThumbnail(uiParent*,uiFlatViewer&);
    			~uiFlatViewThumbnail();

    void		setColors(Color fg,Color bg);

    const uiFlatViewer&	viewer()		{ return viewer_; }

protected:

    void		getUiRect( const uiWorldRect&, uiRect& ) const;

    uiWorldRect*	feedbackwr_;
    uiFlatViewer&	viewer_;
    MouseEventHandler&	mousehandler_;
    Color		fgcolor_;
    Color		bgcolor_;

    void		reDrawHandler(uiRect);
    void		vwChg(CallBacker*);
    void		mouseRelCB(CallBacker*);
    void		mousePressCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);
};

#endif
