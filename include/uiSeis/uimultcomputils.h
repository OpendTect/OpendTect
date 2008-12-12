#ifndef uimultcompdlg_h
#define uimultcompdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          August 2008
 RCS:           $Id: uimultcomputils.h,v 1.2 2008-12-12 09:30:18 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class LineKey;
class uiLabeledSpinBox;

class uiMultCompDlg : public uiDialog
{
public:
			uiMultCompDlg(uiParent*,LineKey);

    int			getCompNr() const;
    bool		needDisplay() const		{ return needdisplay_; }

protected:

    uiLabeledSpinBox*	compfld_;
    bool		needdisplay_;
};


#endif
