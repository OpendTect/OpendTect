#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: uistratlaymoddisp.h,v 1.1 2010-10-22 13:50:07 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
class uiTextItem;
namespace Strat { class LayerModel; }


mClass uiStratLayerModelDisp : public uiGraphicsView
{
public:

    				uiStratLayerModelDisp(uiParent*,
						    const Strat::LayerModel&);
    				~uiStratLayerModelDisp();

protected:

    const Strat::LayerModel&	lm_;
    uiTextItem*		emptyitm_;

    void		eraseAll();
    void		reDraw(CallBacker*);
    void		usrClickCB(CallBacker*);

    void		doDraw();

};


#endif
