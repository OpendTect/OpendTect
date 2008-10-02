#ifndef uimultcompdlg_h
#define uimultcompdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          August 2008
 RCS:           $Id: uimultcomputils.h,v 1.1 2008-10-02 08:54:44 cvshelene Exp $
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

    static int		getNrCompAvail(LineKey);

protected:

    uiLabeledSpinBox*	compfld_;
    bool		needdisplay_;
};


#endif
