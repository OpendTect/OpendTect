#ifndef uiflatviewthumbnail_h
#define uiflatviewthumbnail_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2007
 RCS:           $Id: uiflatviewthumbnail.h,v 1.7 2012-08-03 13:00:58 cvskris Exp $
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigraphicsview.h"
class uiFlatViewer;
class uiRectItem;
class MouseEventHandler;


/*!\brief Shows a thumbnail with current position of a uiFlatViewer. */

mClass(uiFlatView) uiFlatViewThumbnail : public uiGraphicsView
{
public:
    			uiFlatViewThumbnail(uiParent*,uiFlatViewer&);
    			~uiFlatViewThumbnail();

    void		setColors(Color fg,Color bg);

    void		draw();
    void		draw(const uiWorldRect&);
    const uiFlatViewer&	viewer()		{ return viewer_; }

protected:

    void		getUiRect( const uiWorldRect&, uiRect& ) const;

    uiWorldRect*	feedbackwr_;
    uiFlatViewer&	viewer_;
    MouseEventHandler&	mousehandler_;
    Color		fgcolor_;
    Color		bgcolor_;

    uiRectItem*		bgrectitem_;
    uiRectItem*		fgrectitem_;

    void		reDrawHandler(uiRect);
    void		vwChg(CallBacker*);
    void		vwChging(CallBacker*);
    void		mouseRelCB(CallBacker*);
    void		mousePressCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);
};

#endif

