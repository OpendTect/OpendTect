#ifndef uiconvpos_h
#define uiconvpos_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2001
 RCS:           $Id: uiconvpos.h,v 1.4 2007-08-10 12:17:34 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class SurveyInfo;

class uiConvertPos : public uiDialog
{

public:
                        uiConvertPos(uiParent*,SurveyInfo*);

private:

    uiGenInput*         inlfld;
    uiGenInput*         crlfld;
    uiGenInput*         xfld;
    uiGenInput*         yfld;
    void		getCoord();
    void		getBinID();
    SurveyInfo*		survinfo;
};

#endif
