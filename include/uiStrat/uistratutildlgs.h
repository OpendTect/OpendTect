#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          August 2007
 RCS:           $Id: uistratutildlgs.h,v 1.1 2007-08-21 12:40:10 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiLabeledListBox;
class uiPushButton;

/*!\brief Displays a dialog to create new stratigraphic unit */

class uiStratUnitDlg : public uiDialog
{
public:

			uiStratUnitDlg(uiParent*,bool);

protected:
    uiPushButton*	sellithbut_;
    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiGenInput*		unitlithfld_;
    uiGenInput*		insertbafld_;

    void		selLithCB(CallBacker*);

};


/*!\brief Displays a dialog to create new lithology */

class uiLithoDlg : public uiDialog
{
public:

			uiLithoDlg(uiParent*);

protected:
    uiLabeledListBox*	listlithfld_;
    uiPushButton*	newlithbut_;
    uiGenInput*		lithnmfld_;

    void		newLithCB(CallBacker*);
};

#endif
