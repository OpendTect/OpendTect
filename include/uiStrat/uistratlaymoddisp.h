#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: uistratlaymoddisp.h,v 1.5 2010-12-07 16:16:02 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiTextItem;
class uiSpinBox;
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

    Notifier<uiStratLayerModelDisp>	dispEachChg;

protected:

    const Strat::LayerModel& lm_;

    uiGraphicsView*	gv_;
    uiAxisHandler*	xax_;
    uiAxisHandler*	yax_;
    uiTextItem*		emptyitm_;
    uiGenInput*		qtyfld_;
    uiSpinBox*		eachfld_;
    uiComboBox*		lvlfld_;
    uiGraphicsItemSet&	logblckitms_;
    uiGraphicsItemSet&	lvlitms_;

    uiGraphicsScene&	scene();
    void		eraseAll();
    void		dispEachChgd(CallBacker*);
    void		reDraw(CallBacker*);
    void		usrClickCB(CallBacker*);
    void		setDispPars(CallBacker*);
    void		saveMdl(CallBacker*);

    void		doDraw();
    int			dispprop_;
    int			dispeach_;
    bool		fillmdls_;
    Interval<float>	zrg_;
    Interval<float>	vrg_;
    void		getBounds();
    void		drawModel(TypeSet<uiPoint>&,int);
    void		drawLevels();

};


#endif
