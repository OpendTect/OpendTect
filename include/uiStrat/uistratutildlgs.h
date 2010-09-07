#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
 RCS:           $Id: uistratutildlgs.h,v 1.19 2010-09-07 16:03:06 cvsbruno Exp $
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
class uiSpinBox;
class uiTable;
namespace Strat { class Lithology; class UnitRepository; }

/*!\brief Displays a dialog to create/edit a new stratigraphic unit */

mClass uiStratUnitEditDlg : public uiDialog
{
public:
			uiStratUnitEditDlg(uiParent*,Strat::UnitRef&);

    BufferString& 	getLithology() 	{ return lithnm_; }
    void 		setLithology(const BufferString& lith) 	
    			{ lithnm_ = lith; }

protected:

    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiGenInput*		unitlithfld_;
    uiSpinBox*		agestartfld_;
    uiSpinBox*		agestopfld_;
    uiColorInput*	colfld_;
    
    Strat::UnitRef& 	unit_;
    BufferString	entrancename_;
    BufferString	lithnm_;

    void		getFromScreen();
    void		putToScreen();

    void		selLithCB(CallBacker*);
    bool		acceptOK(CallBacker*);
};


/*!\brief Displays a Table to create new units from an existing one */

mClass uiStratUnitDivideDlg : public uiDialog
{
public:
				uiStratUnitDivideDlg(uiParent*,
						const Strat::UnitRef&);

    void			gatherUnits(ObjectSet<Strat::UnitRef>&); 

protected :

    uiTable*                    table_;
    const Strat::UnitRef& 	parentunit_;

    bool			areTimesOK(ObjectSet<Strat::UnitRef>&) const;

    void			setUnit(int,const Strat::UnitRef&);
    void			mouseClick(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			resetUnits(CallBacker*);
};



/*!\brief Displays a dialog to create new lithology */

mClass uiStratLithoDlg : public uiDialog
{
public:

			uiStratLithoDlg(uiParent*);

    const char*		getLithName() const;
    void		setSelectedLith(const char*);

protected:

    uiListBox*		selfld_;
    uiGenInput*		nmfld_;
    uiCheckBox*		isporbox_;

    Strat::Lithology*	prevlith_;
    Strat::UnitRepository& stratrepos_;

    void		fillLiths();
    void		newLith(CallBacker*);
    void		selChg(CallBacker*);
    void		rmSel(CallBacker*);
    void		renameCB(CallBacker*);

    bool		acceptOK(CallBacker*);
};


mClass uiStratLevelDlg : public uiDialog
{
public:

    uiStratLevelDlg(uiParent*);

    void                setLvlInfo(const char*);

protected:

    BufferString        oldlvlnm_;

    uiGenInput*         lvlnmfld_;
    uiColorInput*       lvlcolfld_;

    bool                acceptOK(CallBacker*);
};


mClass uiStratLinkLvlUnitDlg : public uiDialog
{
public:

    			uiStratLinkLvlUnitDlg(uiParent*,Strat::UnitRef*);

    int 		lvlid_;

protected:

    Strat::UnitRef*	unit_;

    uiGenInput*         lvllistfld_;
    TypeSet<int>	ids_;

    bool		acceptOK(CallBacker*);
};


#endif
