#ifndef uistratsimplelaymoddisp_h
#define uistratsimplelaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2012
 RCS:		$Id: uistratsimplelaymoddisp.h,v 1.1 2012-01-17 11:12:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uistratlaymoddisp.h"
class uiLineItem;
class uiTextItem;
class uiRectItem;
class uiAxisHandler;
class uiGraphicsView;
class uiGraphicsScene;
class uiGraphicsItemSet;


mClass uiStratSimpleLayerModelDisp : public uiStratLayerModelDisp
{
public:

    			uiStratSimpleLayerModelDisp(uiStratLayModEditTools&,
					    const Strat::LayerModel&);
    			~uiStratSimpleLayerModelDisp();

    virtual void	modelChanged();
    virtual void	setZoomBox(const uiWorldRect&);

    Color		levelColor() const		{ return lvlcol_; }
    bool&		fillLayerBoxes()		{ return fillmdls_; }
    bool&		useLithColors()			{ return uselithcols_; }

protected:

    uiGraphicsView*	gv_;
    uiAxisHandler*	xax_;
    uiAxisHandler*	yax_;
    uiTextItem*		emptyitm_;
    uiRectItem*		zoomboxitm_;
    uiGraphicsItemSet&	logblckitms_;
    uiGraphicsItemSet&	lvlitms_;
    uiLineItem*		selseqitm_;

    Color		lvlcol_;
    int			dispprop_;
    int			dispeach_;
    bool		fillmdls_;
    int			selseqidx_;
    int			selectedlevel_;
    bool		uselithcols_;
    bool		showzoomed_;
    Interval<float>	vrg_;

    uiGraphicsScene&	scene();
    void		doDraw();
    void		eraseAll();
    void		reDrawCB(CallBacker*);
    void		usrClicked(CallBacker*);
    void		colsToggled(CallBacker*);
    void		showZoomedToggled(CallBacker*);
    void		getBounds();
    void		drawModel(TypeSet<uiPoint>&,int);
    void		drawLevels();
    virtual void	drawSelectedSequence();
    void		updZoomBox();
    int			getXPix(int,float) const;
    bool		isDisplayedModel(int) const;

};


#endif
