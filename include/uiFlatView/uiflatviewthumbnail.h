#ifndef uiflatviewthumbnail_h
#define uiflatviewthumbnail_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Aug 2007
 RCS:           $Id: uiflatviewthumbnail.h,v 1.1 2007-08-23 15:27:31 cvsbert Exp $
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

    void		setColors(Color fg,Color bg);

    const uiFlatViewer&	viewer()		{ return viewer_; }


protected:

    uiFlatViewer&	viewer_;
    MouseEventHandler&	mousehandler_;
    Color		fgcolor_;
    Color		bgcolor_;

    void		reDrawHandler(uiRect);
    void		vwChg(CallBacker*);
    void		mouseRelCB(CallBacker*);
};

#endif
