#ifndef uiconvpos_h
#define uiconvpos_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.h,v 1.1 2001-07-27 10:27:49 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiGenInput;
class uiPushButton;
class SurveyInfo;

class uiConvertPos : public uiDialog
{

public:
                        uiConvertPos(uiParent*,SurveyInfo*);

private:

    uiPushButton*       docoordbut;
    uiPushButton*       dobinidbut;
    uiGenInput*         inlfld;
    uiGenInput*         crlfld;
    uiGenInput*         xfld;
    uiGenInput*         yfld;
    void		getCoord();
    void		getBinID();
    SurveyInfo*		survinfo;
};

#endif
