#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "multiid.h"

class IOObj;
class uiBatchJobDispatcherSel;
class uiGenInput;
class uiListBox;
class uiListBoxFilter;
class uiSeisTransfer;
class uiSeisSel;
class uiToolButton;

/*!\brief Merges cubes in a separate batch process. The order of the selected
	  cubes determines which cube is used first for duplicate traces. */

mExpClass(uiSeis) uiMergeSeis : public uiDialog
{ mODTextTranslationClass(uiMergeSeis);
public:
			uiMergeSeis(uiParent*,
				    const TypeSet<MultiID>* selids=nullptr);
			~uiMergeSeis();

    static const char*	mergeSeisProgName();

protected:

    uiListBox*		availablefld_;
    uiListBoxFilter*	availablefilter_;
    uiListBox*		selectedfld_;
    uiToolButton*	addbut_;
    uiToolButton*	removebut_;
    uiToolButton*	moveupbut_;
    uiToolButton*	movedownbut_;
    uiGenInput*		stackfld_;
    uiSeisTransfer*	transffld_;
    uiSeisSel*		outfld_;
    uiBatchJobDispatcherSel* batchfld_;
    TypeSet<MultiID>	availableids_;
    TypeSet<MultiID>	selectedids_;

    void		fillAvailableCubes(
					const TypeSet<MultiID>* selids=nullptr);
    void		insertAvailable(const MultiID&);
    void		addCubeCB(CallBacker*);
    void		removeCubeCB(CallBacker*);
    void		moveCubeCB(CallBacker*);
    void		selectionChangedCB(CallBacker*);
    void		refreshAvailableList();
    void		updateSelectedList(int curidx=-1);
    void		updateButtonSensitivity();
    void		updateTransfld();
    bool		acceptOK(CallBacker*) override;
    bool		getInput(ObjectSet<IOPar>&,IOPar&);
};
