#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
 RCS:           $Id: uistratutildlgs.h,v 1.21 2010-09-29 16:16:56 cvsbruno Exp $
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
namespace Strat { class Lithology; class LithologySet; }

/*!\brief Displays a dialog to create/edit a new stratigraphic unit */


mClass uiStratUnitEditDlg : public uiDialog
{
public:
			uiStratUnitEditDlg(uiParent*,Strat::NodeUnitRef&);

    void 		setLithology(const BufferString& lith) 	
    					{ lithnm_ = lith; }

    BufferString& 	getLithology() 	{ return lithnm_; }

protected:

    Strat::NodeUnitRef& unit()		{ return unit_; }

    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiColorInput*	colfld_;
    uiGenInput*		unitlithfld_;
    uiSpinBox*		agestartfld_;
    uiSpinBox*		agestopfld_;
    
    Strat::NodeUnitRef& unit_;

    BufferString	entrancename_;
    BufferString	lithnm_;

    void		getFromScreen();
    void		putToScreen();

    bool		acceptOK(CallBacker*);
    void		selLithCB(CallBacker*);
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
    Strat::LithologySet& lithos_;

    void		fillLiths();
    void		newLith(CallBacker*);
    void		selChg(CallBacker*);
    void		rmLast(CallBacker*);
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

    			uiStratLinkLvlUnitDlg(uiParent*,Strat::LeavedUnitRef*);

    int 		lvlid_;

protected:

    Strat::LeavedUnitRef*	unit_;

    uiGenInput*         lvllistfld_;
    TypeSet<int>	ids_;

    bool		acceptOK(CallBacker*);
};


#endif
