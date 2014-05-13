#ifndef uiflatviewthumbnail_h
#define uiflatviewthumbnail_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigraphicsview.h"

class uiFlatViewer;
class uiRectItem;
class MouseEventHandler;

/*!
\brief Shows a thumbnail with current position of a uiFlatViewer.
*/

mExpClass(uiFlatView) uiFlatViewThumbnail : public uiGraphicsView
{
public:
    			uiFlatViewThumbnail(uiParent*,uiFlatViewer&);
    			~uiFlatViewThumbnail();

    void		setColors(Color fg,Color bg);

    void		draw();
    const uiFlatViewer&	viewer()		{ return viewer_; }

protected:

    void		getUiRect( const uiWorldRect&, uiRect& ) const;

    uiPoint		mousedownpt_;
    uiWorldRect*	feedbackwr_;
    uiFlatViewer&	viewer_;
    MouseEventHandler&	mousehandler_;
    Color		fgcolor_;
    Color		bgcolor_;

    uiRectItem*		bgrectitem_;
    uiRectItem*		fgrectitem_;

    void		updateWorldRect();

    void		vwChg(CallBacker*);
    void		mouseRelCB(CallBacker*);
    void		mousePressCB(CallBacker*);
    void		mouseMoveCB(CallBacker*);
};

#endif

