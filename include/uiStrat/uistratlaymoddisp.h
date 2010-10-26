#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: uistratlaymoddisp.h,v 1.2 2010-10-26 15:13:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigraphicsview.h"
class uiTextItem;
class uiAxisHandler;
class BufferStringSet;
namespace Strat { class LayerModel; }


mClass uiStratLayerModelDisp : public uiGraphicsView
{
public:

    			uiStratLayerModelDisp(uiParent*,
					    const Strat::LayerModel&);
    			~uiStratLayerModelDisp();

    void		modelChanged()		{ reDraw(0); }
    void		getDispProperties(BufferStringSet&) const;
    int			dispProp() const 	{ return dispprop_; }
    void		setDispProp( int dp )	{ dispprop_ = dp; }

protected:

    const Strat::LayerModel& lm_;
    int			dispprop_;
    uiTextItem*		emptyitm_;

    void		eraseAll();
    void		reDraw(CallBacker*);
    void		usrClickCB(CallBacker*);

    void		doDraw();
    Interval<float>	zrg_;
    Interval<float>	vrg_;
    void		getBounds();
    uiAxisHandler*	xax_;
    uiAxisHandler*	yax_;

};


#endif
