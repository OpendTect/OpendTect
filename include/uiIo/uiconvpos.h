#ifndef uiconvpos_h
#define uiconvpos_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.h,v 1.2 2001-11-15 08:08:25 nanne Exp $
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
