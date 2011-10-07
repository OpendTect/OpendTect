#ifndef uistratlaymoddisp_h
#define uistratlaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2010
 RCS:		$Id: uistratlaymoddisp.h,v 1.11 2011-10-07 15:09:34 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class uiLineItem;
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
    void		selectSequence(int seqidx);

    bool&		fillLayerBoxes()		{ return fillmdls_; }
    bool&		useLithColors()			{ return uselithcols_; }

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
    uiLineItem*		selseqitm_;

    uiGraphicsScene&	scene();
    void		eraseAll();
    void		dispEachChgd(CallBacker*);
    void		lvlChgd(CallBacker*);
    void		reDrawCB(CallBacker*);
    void		usrClicked(CallBacker*);
    void		colsToggled(CallBacker*);
    void		showZoomedToggled(CallBacker*);

    void		doDraw();
    int			dispprop_;
    int			dispeach_;
    bool		fillmdls_;
    int			selseqidx_;
    bool		uselithcols_;
    bool		showunzoomed_;
    Interval<float>	zrg_;
    Interval<float>	vrg_;
    void		getBounds();
    void		drawModel(TypeSet<uiPoint>&,int);
    void		drawLevels();
    void		drawSelectedSequence();
    void		updZoomBox();
    int			getXPix(int,float) const;
    bool		haveAnyZoom() const;
    bool		isDisplayedModel(int) const;

};


#endif
