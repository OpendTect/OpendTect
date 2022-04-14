#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck / Bruno
 Date:          July 2007 /Sept 2010
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uilistbox.h"

mExpClass(uiStrat) uiStratLvlList : public uiListBox
{ mODTextTranslationClass(uiStratLvlList)
public:

		uiStratLvlList(uiParent*);
		~uiStratLvlList();

    void	setLevels();
    void	setIsLocked( bool yn ) { islocked_ = yn; }

    bool	anyChg() const	{ return anychange_; }
    void	setNoChg()	{ anychange_ = false; }

protected:

    bool	islocked_ = false;
    bool	anychange_ = false;

    void	fill();
    void	editLevel(bool);
    bool	checkLocked() const;

    void	lvlSetChgCB(CallBacker*);
    void	removeLvl(CallBacker*);
    void	selLvlChgCB(CallBacker*);

    void	addCB(CallBacker*);
    void	editCB(CallBacker*);
    void	removeCB(CallBacker*);
    void	removeAllCB(CallBacker*);

private:

    static const char*	sNoLevelTxt()		{return "--- None ---"; }
};

