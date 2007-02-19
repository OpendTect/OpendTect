#ifndef uiflatviewcontrol_h
#define uiflatviewcontrol_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2007
 RCS:           $Id: uiflatviewcontrol.h,v 1.1 2007-02-19 16:41:45 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiButton;
class uiButtonGroup;
class uiFlatViewer;


/*!\brief Fulfills the FlatDisp::Viewer specifications for vertical data. */

class uiFlatViewControl : public uiGroup
{
public:

    struct Setup
    {
			Setup( bool vert=true )
			    : vertical_(vert)
			    , withstates_(true)		{}

	mDefSetupMemb(bool,	vertical)
	mDefSetupMemb(bool,	withstates)
    };

    			uiFlatViewControl(uiFlatViewer&,const Setup&);
			~uiFlatViewControl();

    Notifier<uiFlatViewControl>	posChange;

protected:

    uiFlatViewer&	vwr_;
    const Setup&	setup_;

    uiButtonGroup*	posgrp_;
    uiButtonGroup*	stategrp_;
    uiButtonGroup*	parsgrp_;

    uiButton*		zoominbut_;
    uiButton*		zoomoutbut_;
    uiButton*		panupbut_;
    uiButton*		panleftbut_;
    uiButton*		panrightbut_;
    uiButton*		pandownbut_;
    uiButton*		manipbut_;
    uiButton*		drawbut_;
    uiButton*		parsbut_;

    void		zoomCB(CallBacker*);
    void		panCB(CallBacker*);
    void		stateCB(CallBacker*);
    void		parsCB(CallBacker*);

};

#endif
