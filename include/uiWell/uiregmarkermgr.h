#pragma once
/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"

#include "welldata.h"
#include "stratlevel.h"

#include <unordered_set>

class uiListBox;
class uiListBoxFilter;
class uiPushButton;
class uiRegMarkerList;
class uiToolButton;

namespace Strat { class RepositoryAccess; }

mExpClass(uiWell) uiRegionalMarkerMgr : public uiDialog
{ mODTextTranslationClass(uiRegionalMarkerMgr)
public:
				uiRegionalMarkerMgr(uiParent*,
						    ObjectSet<Well::Data>&);
				~uiRegionalMarkerMgr();

private:

    void			initDlg(CallBacker*);
    bool			acceptOK(CallBacker*) override;
    bool			rejectOK(CallBacker*) override;

    void			regMarkerSelectedCB(CallBacker*);
    void			regMarkerRemovedCB(CallBacker*);
    void			allRegMarkersRemovedCB(CallBacker*);
    void			createNewEqMarkerSetCB(CallBacker*);
    void			moveToNewEqMarkerSetCB(CallBacker*);
    void			addAsEqMarkerCB(CallBacker*);
    void			removeFromEqMarkerCB(CallBacker*);
    void			removeAllEquivalentMarkersCB(CallBacker*);
    void			changeRegionalMarkerCB(CallBacker*);
    void			makeRegionalMarkerCB(CallBacker*);
    void			eqvMarkerChosenCB(CallBacker*);

    void			initLevelSet();
    void			readTree();
    void			fillLists();
    void			fillRegionalMarkersList();
    void			fillAllMarkersList();
    void			updatePriorityButtonState(bool makesensitive);

    void			modifyEqvMarkerPriority(CallBacker*);
    void			addToEquivalentMarkerSet(const char*,
						    const BufferStringSet&);
    void			setAsRegionalMarker(const BufferStringSet&);
    void			replaceRegionalMarker(const BufferString&);
    void			setEqvMarkerAsRegional(const BufferString&);

    bool			isInputOK(uiString&) const;
    void			resetRegMarkers();
    void			saveWells(const std::unordered_set<int>&);

    struct RegMarker
    {
    public:

				RegMarker(const Strat::Level&);
				~RegMarker();

	void			operator =(const RegMarker&);
	bool			operator ==(const RegMarker&) const;
	void			addAsEquivalentMarker(const char*,
						      bool attop=false);
	void			addAsEquivalentMarkers(const BufferStringSet&);
	void			removeFromEquivalentMarker(
						       const BufferStringSet&);
	void			setEquivalentMarkersEmpty();
	void			setLevel(const Strat::Level&);
	void			modifyPriority(int idx,bool increase);

	Strat::LevelID		levelID() const;
	BufferString		name() const;

	int			nrEquivalentMarkers() const;
	bool			isEquivalent(const char*) const;
	void			getEquivalentMarkerNames(BufferStringSet&,
						bool inclregmarker=true) const;

    private:

	const Strat::Level&		level_;
	BufferStringSet			eqmarkers_;
    };

    uiListBox*				allmrkrlist_;
    uiListBoxFilter*			allmrkrsfilter_;
    uiListBox*				eqvmarkerlist_;
    uiRegMarkerList*			regmlist_;
    uiPushButton*			eqvtoregbut_;
    uiToolButton*			eqaddnewbut_;
    uiToolButton*			incprioritybut_;
    uiToolButton*			decprioritybut_;
    uiToolButton*			allmrkrsaddnewbut_;
    uiPushButton*			allmarkerstoregbut_;

    BufferStringSet			allmarkerlistnames_;
    BufferStringSet			allmarkernames_;
    ManagedObjectSet<RegMarker>		regmarkers_;
    RefObjectSet<Well::Data>		wds_;
    TypeSet<Strat::LevelID>		removedrgms_;
};


mClass(uiWell) uiChooseRegMarkerDlg : public uiDialog
{ mODTextTranslationClass(uiChooseRegMarkerDlg)
public:
				uiChooseRegMarkerDlg(uiParent*,
						     const BufferStringSet&);

    BufferString		chosenMarkerName() const { return markernm_; }

private:

    bool			acceptOK(CallBacker*) override;

    uiListBox*			list_;
    BufferString		markernm_;

};
