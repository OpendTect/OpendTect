#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          August 2007
 RCS:           $Id: uistratutildlgs.h,v 1.5 2007-09-26 15:24:19 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class BufferStringSet;
class uiColorInput;
class uiGenInput;
class uiLabeledListBox;
class uiStratMgr;

/*!\brief Displays a dialog to create new stratigraphic unit */

class uiStratUnitDlg : public uiDialog
{
public:

			uiStratUnitDlg(uiParent*,uiStratMgr*);
    const char*		getUnitName() const;	
    const char*		getUnitDesc() const;
    const char*		getUnitLith() const;

protected:
    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiGenInput*		unitlithfld_;

    uiStratMgr*		uistratmgr_;

    void		selLithCB(CallBacker*);
    bool		acceptOK(CallBacker*);

};


/*!\brief Displays a dialog to create new lithology */

class uiLithoDlg : public uiDialog
{
public:

			uiLithoDlg(uiParent*, uiStratMgr*);
    const char*		getLithName() const;
    void		setSelectedLith(const char*);

protected:
    uiLabeledListBox*	listlithfld_;
    uiGenInput*		lithnmfld_;
    uiStratMgr*		uistratmgr_;

    void		newLithCB(CallBacker*);
};


/*!\brief Displays a dialog to create new level */

class uiStratLevelDlg : public uiDialog
{
public:

			uiStratLevelDlg(uiParent*,uiStratMgr*);

    void		setLvlInfo(const char*);
protected:

    BufferString	oldlvlnm_;
    uiStratMgr*		uistratmgr_;

    uiGenInput*		lvlnmfld_;
    uiGenInput*		lvltvstrgfld_;
    uiGenInput*		lvltimergfld_;
    uiGenInput*		lvltimefld_;
    uiColorInput*	lvlcolfld_;
    void		isoDiaSel(CallBacker*);
    bool		acceptOK(CallBacker*);
};


/*!\brief Displays a dialog to link level and stratigraphic unit*/

class uiStratLinkLvlUnitDlg : public uiDialog
{
public:

			uiStratLinkLvlUnitDlg(uiParent*,const char*,
					      uiStratMgr*);
    uiGenInput*		lvltoplistfld_;
    uiGenInput*		lvlbaselistfld_;

protected:

    bool		acceptOK(CallBacker*);
    
    uiStratMgr*		uistratmgr_;
};


#endif
