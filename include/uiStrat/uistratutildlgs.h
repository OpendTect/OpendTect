#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
 RCS:           $Id: uistratutildlgs.h,v 1.26 2010-10-07 12:11:01 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "ranges.h"
#include "stratunitref.h"
#include "uilistbox.h"

class BufferStringSet;
class uiColorInput;
class uiGenInput;
class uiListBox;
class uiCheckBox;
class uiStratMgr;
class uiSpinBox;
class uiTable;
namespace Strat { class Lithology; }

/*!\brief Displays a dialog to create/edit a new stratigraphic unit */

mClass uiStratLithoBox : public uiListBox
{
public:
    			uiStratLithoBox(uiParent*);
protected:

    void		selChanged(CallBacker*);
    void		fillLiths(CallBacker*);
};


mClass uiStratUnitEditDlg : public uiDialog
{
public:
			uiStratUnitEditDlg(uiParent*,Strat::NodeUnitRef&);

    const TypeSet<int>& getLithologies() const 	{ return lithids_; }

protected:

    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiColorInput*	colfld_;
    uiSpinBox*		agestartfld_;
    uiSpinBox*		agestopfld_;
    uiStratLithoBox*	unitlithfld_;
    
    Strat::NodeUnitRef& unit_;

    BufferString	entrancename_;
    TypeSet<int>	lithids_;

    void		getFromScreen();
    void		putToScreen();

    bool		acceptOK(CallBacker*);
    void		selLithCB(CallBacker*);
};


mClass uiStratLithoDlg : public uiDialog
{
public:

			uiStratLithoDlg(uiParent*);

    const char*		getLithName() const;
    void		setSelectedLith(const char*);

protected:

    uiStratLithoBox*	selfld_;
    uiGenInput*		nmfld_;
    uiCheckBox*		isporbox_;

    Strat::Lithology*	prevlith_;

    void		newLith(CallBacker*);
    void		selChg(CallBacker*);
    void		rmLast(CallBacker*);
    void		renameCB(CallBacker*);

    bool		acceptOK(CallBacker*);
};



/*!\brief Displays a Table to create new units from an existing one */

mClass uiStratUnitDivideDlg : public uiDialog
{
public:
				uiStratUnitDivideDlg(uiParent*,
						const Strat::LeavedUnitRef&);

    void			gatherUnits(ObjectSet<Strat::LeavedUnitRef>&); 

protected :

    uiTable*                    table_;
    const Strat::LeavedUnitRef& rootunit_;

    bool			areTimesOK(
	    				ObjectSet<Strat::LeavedUnitRef>&) const;

    void			addUnitToTable(int,const Strat::LeavedUnitRef&);
    void			mouseClick(CallBacker*);
    bool			acceptOK(CallBacker*);
    void			resetUnits(CallBacker*);
};



/*!\brief Displays a dialog to create new lithology */

mClass uiStratLevelDlg : public uiDialog
{
public:

    uiStratLevelDlg(uiParent*);

    void                setLvlInfo(const char*,const Color& col);
    void		getLvlInfo(BufferString&,Color& col) const;

protected:

    uiGenInput*         lvlnmfld_;
    uiColorInput*       lvlcolfld_;
};


mClass uiStratLinkLvlUnitDlg : public uiDialog
{
public:

    			uiStratLinkLvlUnitDlg(uiParent*,Strat::LeavedUnitRef&);

    int 		lvlid_;

protected:

    Strat::LeavedUnitRef& unit_;

    uiGenInput*         lvllistfld_;
    TypeSet<int>	ids_;

    bool		acceptOK(CallBacker*);
};


#endif
