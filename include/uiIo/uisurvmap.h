#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.13 2008-09-01 07:26:13 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigraphicsview.h"
class uiGraphicsScene;
class SurveyInfo;


class uiSurveyMap : public uiGraphicsView
{
public:
			uiSurveyMap(uiParent*);
    void		drawMap(const SurveyInfo*);
    void		removeItems();
    void		setWidth( int w )	{ width_ = w; }
    void		setHeight( int h )	{ height_ = h; }
protected:
    uiGraphicsScene*	mapscene_;
    int			width_;
    int			height_;
};


class uiSurveyMapDlg : public uiDialog
{
public:

			uiSurveyMapDlg(uiParent*);
    void		doCanvas(CallBacker*);

    uiGraphicsScene*	scene_;
    uiGraphicsView*	view_;
};

#endif
