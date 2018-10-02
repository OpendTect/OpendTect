#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck / Bert
 Date:          September 2007 / Aug 2018
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "stratlevel.h"

#include "uigroup.h"
#include "uidialog.h"

class uiComboBox;
class uiListBox;


/*!\brief Selector for stratigraphic levels */

mExpClass(uiTools) uiStratLevelSel : public uiGroup
{ mODTextTranslationClass(uiStratLevelSel)
public:

    typedef Strat::Level Level;
    typedef Level::ID	LevelID;

			uiStratLevelSel(uiParent*,bool withudf,
				    const uiString& lbltxt=sTiedToTxt());
			~uiStratLevelSel();

    Level		selected() const;
    BufferString	getLevelName() const;
    Color		getColor() const;
    LevelID		getID() const;

    void		setName(const char*);
    void		setID(LevelID);
    void		setToolTip(const uiString&);

    Notifier<uiStratLevelSel> selChange;

    static uiString	sTiedToTxt();

    uiComboBox*		box()			{ return fld_; }

protected:

    uiComboBox*		fld_;
    const bool		haveudf_;

    void		addItem(const char*,const Color&);
    void		fill();

    void		selCB(CallBacker*);
    void		extChgCB(CallBacker*);
};


/*!\brief Selection dialog for one or more stratigraphic levels */

mExpClass(uiTools) uiStratLevelSelDlg : public uiDialog
{ mODTextTranslationClass(uiStratLevelSelDlg)
public:

    typedef Strat::Level Level;
    typedef Level::ID	LevelID;

			uiStratLevelSelDlg(uiParent*,const uiString&,
					OD::ChoiceMode cm=OD::ChooseOnlyOne);
			~uiStratLevelSelDlg();

    Level		selected() const;
    BufferString	getLevelName() const;
    Color		getColor() const;
    LevelID		getID() const;

    void		setName(const char*);
    void		setID(LevelID);

    Notifier<uiStratLevelSelDlg> selChange;

    uiListBox*		box()			{ return fld_; }

protected:

    uiListBox*		fld_;

    void		addItem(const char*,const Color&);
    void		fill();

    void		selCB(CallBacker*);
    void		extChgCB(CallBacker*);
};
