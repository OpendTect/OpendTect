#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"

#include "uidialog.h"
#include "uilistbox.h"
#include "uistring.h"
#include "uitable.h"
#include "ranges.h"
#include "stratunitref.h"

class uiColorInput;
class uiGenInput;
class uiListBox;
class uiCheckBox;
class uiStratMgr;
class uiSpinBox;
class uiTable;


/*!\brief Displays a dialog to create/edit a new stratigraphic unit */

mExpClass(uiStrat) uiStratLithoBox : public uiListBox
{ mODTextTranslationClass(uiStratLithoBox)
public:
			uiStratLithoBox(uiParent*);
			uiStratLithoBox(uiParent*,const uiListBox::Setup&);
			~uiStratLithoBox();

protected:

    void		fillLiths(CallBacker*);
};


mExpClass(uiStrat) uiStratUnitEditDlg : public uiDialog
{ mODTextTranslationClass(uiStratUnitEditDlg)
public:
			uiStratUnitEditDlg(uiParent*,Strat::NodeUnitRef&);
			~uiStratUnitEditDlg();

    const TypeSet<Strat::LithologyID>& getLithologies() const
			{ return lithids_; }
    static bool		checkWrongChar(char*);

protected:

    uiGenInput*		unitnmfld_;
    uiGenInput*		unitdescfld_;
    uiColorInput*	colfld_;
    uiSpinBox*		agestartfld_;
    uiSpinBox*		agestopfld_;
    uiStratLithoBox*	unitlithfld_;

    Strat::NodeUnitRef& unit_;

    BufferString	entrancename_;
    TypeSet<Strat::LithologyID> lithids_;

    void		getFromScreen();
    void		putToScreen();

    bool		acceptOK(CallBacker*) override;
    void		selLithCB(CallBacker*);
};


mExpClass(uiStrat) uiStratLithoDlg : public uiDialog
{ mODTextTranslationClass(uiStratLithoDlg)
public:

			uiStratLithoDlg(uiParent*);
			~uiStratLithoDlg();

    const char*		getLithName() const;
    void		setSelectedLith(const char*);
    bool		anyChg() const		{ return anychg_; }

protected:

    uiStratLithoBox*	selfld_;
    uiGenInput*		nmfld_;
    uiCheckBox*		isporbox_;
    uiColorInput*	colfld_;

    Strat::Lithology*	prevlith_ = nullptr;
    bool		anychg_ = false;

    void		selChg(CallBacker*);
    void		propChg(CallBacker*);
    void		newLith(CallBacker*);
    void		rmLast(CallBacker*);
    void		renameCB(CallBacker*);

};



/*!\brief Displays a Table to create new units from an existing one */

mExpClass(uiStrat) uiStratUnitDivideDlg : public uiDialog
{ mODTextTranslationClass(uiStratUnitDivideDlg)
public:
				uiStratUnitDivideDlg(uiParent*,
						const Strat::LeavedUnitRef&);
				~uiStratUnitDivideDlg();

    void			gatherUnits(ObjectSet<Strat::LeavedUnitRef>&);

protected :

    mExpClass(uiStrat) uiDivideTable : public uiTable
    { mODTextTranslationClass(uiDivideTable)
    public:
				uiDivideTable(uiParent*,const uiTable::Setup&);
				~uiDivideTable();
    protected:
	void			popupMenu(CallBacker*) override;
    };


    uiTable*			table_;
    const Strat::LeavedUnitRef& rootunit_;

    bool			areTimesOK(ObjectSet<Strat::LeavedUnitRef>&,
					   uiString&) const;

    void			addUnitToTable(int,const Strat::LeavedUnitRef&);
    void			mouseClick(CallBacker*);
    bool			acceptOK(CallBacker*) override;
    void			resetUnits(CallBacker*);
};



/*!\brief Displays a dialog to create new lithology */

mExpClass(uiStrat) uiStratLevelDlg : public uiDialog
{ mODTextTranslationClass(uiStratLevelDlg)
public:

			uiStratLevelDlg(uiParent*);
			~uiStratLevelDlg();

    void		setLvlInfo(const char*,const OD::Color&);
    void		getLvlInfo(BufferString&,OD::Color&) const;

protected:

    uiGenInput*		lvlnmfld_;
    uiColorInput*	lvlcolfld_;
};


mExpClass(uiStrat) uiStratLinkLvlUnitDlg : public uiDialog
{ mODTextTranslationClass(uiStratLinkLvlUnitDlg)
public:

			uiStratLinkLvlUnitDlg(uiParent*,Strat::LeavedUnitRef&);
			~uiStratLinkLvlUnitDlg();

    Strat::LevelID	lvlid_;

protected:

    Strat::LeavedUnitRef&	unit_;
    uiGenInput*			lvllistfld_;
    TypeSet<Strat::LevelID>	ids_;

    bool		acceptOK(CallBacker*) override;
};


mExpClass(uiStrat) uiStratContentsDlg : public uiDialog
{ mODTextTranslationClass(uiStratContentsDlg)
public:
			uiStratContentsDlg(uiParent*);
			~uiStratContentsDlg();

    bool		anyChg() const		{ return anychg_; }

protected:

    bool		anychg_ = false;

};
