#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene
 Date:          August 2007
 RCS:           $Id: uistratutildlgs.h,v 1.2 2007-08-27 11:52:18 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiLabeledListBox;

/*!\brief Displays a dialog to create new stratigraphic unit */

class uiStratUnitDlg : public uiDialog
{
public:

			uiStratUnitDlg(uiParent*);
    const char*		getUnitName() const;	
    const char*		getUnitDesc() const;
    const char*		getUnitLith() const;

protected:
    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiGenInput*		unitlithfld_;

    void		selLithCB(CallBacker*);

};


/*!\brief Displays a dialog to create new lithology */

class uiLithoDlg : public uiDialog
{
public:

			uiLithoDlg(uiParent*);
    const char*		getLithName() const;
    void		setSelectedLith(const char*);

protected:
    uiLabeledListBox*	listlithfld_;
    uiGenInput*		lithnmfld_;

    void		newLithCB(CallBacker*);
};

#endif
