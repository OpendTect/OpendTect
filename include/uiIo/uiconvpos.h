#ifndef uiconvpos_h
#define uiconvpos_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.h,v 1.3 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiGenInput;
class uiToolButton;
class SurveyInfo;

class uiConvertPos : public uiDialog
{

public:
                        uiConvertPos(uiParent*,SurveyInfo*);

private:

    uiToolButton*       docoordbut;
    uiToolButton*       dobinidbut;
    uiGenInput*         inlfld;
    uiGenInput*         crlfld;
    uiGenInput*         xfld;
    uiGenInput*         yfld;
    void		getCoord();
    void		getBinID();
    SurveyInfo*		survinfo;
};

#endif
