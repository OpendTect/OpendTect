#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.11 2003-03-18 16:05:24 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiCanvas;
class SurveyInfo;


class uiSurveyMap
{
public:

			uiSurveyMap(uiCanvas*);
    void		drawMap(const SurveyInfo*);

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
