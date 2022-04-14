#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uigroup.h"
#include "stratlevel.h"

class uiComboBox;


/*!\brief Selector for stratigraphic levels */

mExpClass(uiTools) uiStratLevelSel : public uiGroup
{ mODTextTranslationClass(uiStratLevelSel)
public:

			uiStratLevelSel(uiParent*,bool withudf,
					const uiString& lbltxt=sTiedToTxt());
					//!< pass null for no label
			~uiStratLevelSel();

    Strat::Level	selected() const;
    BufferString	getLevelName() const;
    OD::Color		getColor() const;
    Strat::Level::ID	getID() const;

    void		setName(const char*);
    void		setID(Strat::Level::ID);
    void		setToolTip(const uiString&);

    Notifier<uiStratLevelSel> selChange;

    static uiString	sTiedToTxt();

    uiComboBox*		box()			{ return fld_; }

protected:

    uiComboBox*		fld_;
    bool		haveudf_;

    void		addItem(const char*,const OD::Color&);
    void		fill();

    void		selCB(CallBacker*);
    void		curSetChgCB(CallBacker*);
    void		extChgCB(CallBacker*);
};


