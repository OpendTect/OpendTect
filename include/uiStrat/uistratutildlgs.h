#ifndef uistratutildlgs_h
#define uistratutildlgs_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2007
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uidialog.h"
#include "uitable.h"
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

mExpClass(uiStrat) uiStratLithoBox : public uiListBox
{
public:
    			uiStratLithoBox(uiParent*);
    			~uiStratLithoBox();
protected:

    void		fillLiths(CallBacker*);
};


mExpClass(uiStrat) uiStratUnitEditDlg : public uiDialog
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


mExpClass(uiStrat) uiStratLithoDlg : public uiDialog
{
public:

			uiStratLithoDlg(uiParent*);

    const char*		getLithName() const;
    void		setSelectedLith(const char*);
    bool		anyChg() const		{ return anychg_; }

protected:

    uiStratLithoBox*	selfld_;
    uiGenInput*		nmfld_;
    uiCheckBox*		isporbox_;
    uiColorInput*	colfld_;

    Strat::Lithology*	prevlith_;
    bool		anychg_;

    void		newLith(CallBacker*);
    void		selChg(CallBacker*);
    void		rmLast(CallBacker*);
    void		renameCB(CallBacker*);

};



/*!\brief Displays a Table to create new units from an existing one */

mExpClass(uiStrat) uiStratUnitDivideDlg : public uiDialog
{
public:
				uiStratUnitDivideDlg(uiParent*,
						const Strat::LeavedUnitRef&);

    void			gatherUnits(ObjectSet<Strat::LeavedUnitRef>&); 

protected :

    mExpClass(uiStrat) uiDivideTable : public uiTable
    {
	public: 	
				uiDivideTable(uiParent* p,
						const uiTable::Setup& s)
				    : uiTable(p,s,"Subdivide unit table")
				{}
	protected:
	virtual void 		popupMenu(CallBacker*);
    };


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

mExpClass(uiStrat) uiStratLevelDlg : public uiDialog
{
public:

    uiStratLevelDlg(uiParent*);

    void                setLvlInfo(const char*,const Color& col);
    void		getLvlInfo(BufferString&,Color& col) const;

protected:

    uiGenInput*         lvlnmfld_;
    uiColorInput*       lvlcolfld_;
};


mExpClass(uiStrat) uiStratLinkLvlUnitDlg : public uiDialog
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


mExpClass(uiStrat) uiStratContentsDlg : public uiDialog
{
public:
			uiStratContentsDlg(uiParent*);
     bool		anyChg() const		{ return anychg_; }

protected:

     bool		anychg_;

};



#endif

