#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
 RCS:           $Id: uistratutildlgs.h,v 1.12 2010-05-07 12:50:46 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"
#include "stratunitref.h"

class BufferStringSet;
class uiColorInput;
class uiGenInput;
class uiListBox;
class uiCheckBox;
class uiStratMgr;
namespace Strat { class Lithology; }

/*!\brief Displays a dialog to create new stratigraphic unit */

mClass uiStratUnitDlg : public uiDialog
{
public:

			uiStratUnitDlg(uiParent*,uiStratMgr*);

    void		setUnitProps(const Strat::UnitRef::Props&);
    void		getUnitProps(Strat::UnitRef::Props&) const;	

protected:

    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiGenInput*		unitlithfld_;
    uiGenInput*		agefld_;
    uiColorInput*	colfld_;
    
    uiStratMgr*		uistratmgr_;

    void		selLithCB(CallBacker*);
    bool		acceptOK(CallBacker*);

};


/*!\brief Displays a dialog to create new lithology */

mClass uiStratLithoDlg : public uiDialog
{
public:

			uiStratLithoDlg(uiParent*, uiStratMgr*);
    const char*		getLithName() const;
    void		setSelectedLith(const char*);

protected:

    uiListBox*		selfld_;
    uiGenInput*		nmfld_;
    uiCheckBox*		isporbox_;

    Strat::Lithology*	prevlith_;
    uiStratMgr*		uistratmgr_;

    void		fillLiths();
    void		newLith(CallBacker*);
    void		selChg(CallBacker*);
    void		rmSel(CallBacker*);
    void		renameCB(CallBacker*);

    bool		acceptOK(CallBacker*);
};


/*!\brief Displays a dialog to create new level */

mClass uiStratLevelDlg : public uiDialog
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

mClass uiStratLinkLvlUnitDlg : public uiDialog
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
