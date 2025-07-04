#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "uilistbox.h"

namespace Strat { class Level; class RegMarker; }

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
    void	levelAddedCB(CallBacker*);
    void	levelRemovedCB(CallBacker*);
    void	removeLvl(CallBacker*);
    void	selLvlChgCB(CallBacker*);

    void	addCB(CallBacker*);
    void	editCB(CallBacker*);
    void	removeCB(CallBacker*);
    void	removeAllCB(CallBacker*);

    void	addLevel(const Strat::Level&);
    void	removeLevel(const Strat::Level&);

private:

    static const char*	sNoLevelTxt()		{return "--- None ---"; }
};


mExpClass(uiStrat) uiRegMarkerList : public uiListBox
{ mODTextTranslationClass(uiRegMarkerList)
public:
		uiRegMarkerList(uiParent*);
		~uiRegMarkerList();

    Notifier<uiRegMarkerList>	regMarkersRemoved;

private:

    void	fill();
    void	fillStratLevels();

    void	removeCB(CallBacker*);
    void	removeAllCB(CallBacker*);
    void	updateLevelCB(CallBacker*);
    void	levelAddedCB(CallBacker*);
    void	levelRemovedCB(CallBacker*);

    void	addRegMarker(const Strat::RegMarker&);
    void	removeRegMarker(const Strat::RegMarker&);

    static const char*	sNoLevelTxt()		{return "--- None ---"; }

};
