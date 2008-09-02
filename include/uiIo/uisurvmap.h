#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.14 2008-09-02 12:49:20 cvsnanne Exp $
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

protected:
    uiGraphicsScene*	mapscene_;
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
