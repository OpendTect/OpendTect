#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          September 2007
________________________________________________________________________

-*/

#include "stratlevel.h"

#include "uitoolsmod.h"
#include "uigroup.h"

class uiComboBox;
namespace Strat { class Level; }


/*!\brief Selector for stratigraphic levels */

mExpClass(uiTools) uiStratLevelSel : public uiGroup
{ mODTextTranslationClass(uiStratLevelSel)
public:

    typedef Strat::Level::ID	LevelID;

			uiStratLevelSel(uiParent*,bool withudf,
				    const uiString& lbltxt=sTiedToTxt());
			~uiStratLevelSel();

    Strat::Level	selected() const;
    BufferString	getLevelName() const;
    Color		getColor() const;
    LevelID		getID() const;

    void		setName(const char*);
    void		setID(LevelID);

    Notifier<uiStratLevelSel> selChange;

    static const uiString sTiedToTxt();

protected:

    uiComboBox*		fld_;
    const bool		haveudf_;

    void		addItem(const char*,const Color&);
    void		fill();

    void		selCB(CallBacker*);
    void		extChgCB(CallBacker*);
};
