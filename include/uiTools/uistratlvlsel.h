#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    Strat::LevelID	getID() const;

    void		setName(const char*) override;
    void		setID(Strat::LevelID);
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
