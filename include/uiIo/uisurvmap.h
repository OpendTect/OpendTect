#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.10 2003-01-16 11:26:25 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiCanvas;
class SurveyInfo;


class uiSurveyMap
{
public:

			uiSurveyMap(uiCanvas*,SurveyInfo*);
    void		drawMap(SurveyInfo*);

    SurveyInfo*		survinfo;
    uiCanvas*		mapcanvas;

};


class uiSurveyMapDlg : public uiDialog
{
public:

			uiSurveyMapDlg(uiParent*);
    void		doCanvas(CallBacker*);

    uiCanvas*		cv;

};

#endif
