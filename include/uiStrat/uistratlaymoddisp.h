#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: uistratlaymoddisp.h,v 1.7 2010-12-21 13:19:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiSpinBox;
class uiTextItem;
class uiRectItem;
class uiGenInput;
class uiComboBox;
class uiAxisHandler;
class uiGraphicsView;
class uiGraphicsScene;
class BufferStringSet;
class uiGraphicsItemSet;
namespace Strat { class LayerModel; }


mClass uiStratLayerModelDisp : public uiGroup
{
public:

    			uiStratLayerModelDisp(uiParent*,
					    const Strat::LayerModel&);
    			~uiStratLayerModelDisp();

    void		modelChanged();
    void		getDispProperties(BufferStringSet&) const;
    int			getEachDisp() const;
    const char*		selectedLevel() const;		//!< null for none
    const TypeSet<float>& levelDepths() const		{ return lvldpths_; }
    Color		levelColor() const		{ return lvlcol_; }
    void		setZoomBox(const uiWorldRect&);

    Notifier<uiStratLayerModelDisp>	dispEachChg;
    Notifier<uiStratLayerModelDisp>	levelChg;

protected:

    const Strat::LayerModel& lm_;
    TypeSet<float>	lvldpths_;
    Color		lvlcol_;
    uiWorldRect		zoomwr_;

    uiGraphicsView*	gv_;
    uiAxisHandler*	xax_;
    uiAxisHandler*	yax_;
    uiTextItem*		emptyitm_;
    uiRectItem*		zoomboxitm_;
    uiGenInput*		qtyfld_;
    uiSpinBox*		eachfld_;
    uiComboBox*		lvlfld_;
    uiGraphicsItemSet&	logblckitms_;
    uiGraphicsItemSet&	lvlitms_;

    uiGraphicsScene&	scene();
    void		eraseAll();
    void		dispEachChgd(CallBacker*);
    void		lvlChgd(CallBacker*);
    void		reDraw(CallBacker*);
    void		usrClickCB(CallBacker*);

    void		doDraw();
    int			dispprop_;
    int			dispeach_;
    bool		fillmdls_;
    Interval<float>	zrg_;
    Interval<float>	vrg_;
    void		getBounds();
    void		drawModel(TypeSet<uiPoint>&,int);
    void		drawLevels();
    void		updZoomBox();
    int			getXPix(int,float) const;

};


#endif
