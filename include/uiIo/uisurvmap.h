#ifndef uisurvmap_h
#define uisurvmap_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uisurvmap.h,v 1.12 2003-11-07 12:21:54 bert Exp $
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
