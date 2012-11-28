#ifndef uistratsimplelaymoddisp_h
#define uistratsimplelaymoddisp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2012
 RCS:		$Id$
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
namespace Strat { class LayerSequence; class Content; }


mClass uiStratSimpleLayerModelDisp : public uiStratLayerModelDisp
{
public:

    			uiStratSimpleLayerModelDisp(uiStratLayModEditTools&,
					    const Strat::LayerModelProvider&);
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
    uiGraphicsItemSet&	contitms_;
    uiLineItem*		selseqitm_;

    Color		lvlcol_;
    int			dispprop_;
    int			dispeach_;
    bool		fillmdls_;
    int			selectedlevel_;
    bool		uselithcols_;
    bool		showzoomed_;
    const Strat::Content* selectedcontent_;
    Interval<float>	vrg_;

    uiGraphicsScene&	scene();
    void		doDraw();
    void		eraseAll();
    void		reDrawCB(CallBacker*);
    void		usrClicked(CallBacker*);
    void		doubleClicked(CallBacker*);
    void		colsToggled(CallBacker*);
    void		showZoomedToggled(CallBacker*);
    int			getClickedModelNr() const;
    void		getBounds();
    void		handleRightClick(int);
    void		drawModel(TypeSet<uiPoint>&,int);
    void		drawLevels();
    virtual void	drawSelectedSequence();
    void		updZoomBox();
    int			getXPix(int,float) const;
    bool		isDisplayedModel(int) const;
    void		removeLayers(Strat::LayerSequence&,int,bool);
    void		forceRedispAll(bool modeledited=false);

    void		mouseMoved(CallBacker*);
};


#endif
