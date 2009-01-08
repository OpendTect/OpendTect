#ifndef uimultcompdlg_h
#define uimultcompdlg_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          August 2008
 RCS:           $Id: uimultcomputils.h,v 1.4 2009-01-08 08:31:03 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class LineKey;
class uiListBox;

mClass uiMultCompDlg : public uiDialog
{
public:
			uiMultCompDlg(uiParent*,LineKey);

    int			getCompNr() const;
    bool		needDisplay() const		{ return needdisplay_; }

protected:

    uiListBox*		compfld_;
    bool		needdisplay_;
};


#endif
