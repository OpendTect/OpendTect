#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class uiCheckBox;
class uiComboBox;
class uiGenInput;
class uiIOObjSel;
class uiListBox;
class uiSeisMultiCubePSEntry;
class IOObj;

mExpClass(uiSeis) uiSeisMultiCubePS : public uiDialog
{ mODTextTranslationClass(uiSeisMultiCubePS);

public:
			uiSeisMultiCubePS(uiParent*,const MultiID&);
			~uiSeisMultiCubePS();

    const IOObj*	createdIOObj() const;

protected:

    ObjectSet<uiSeisMultiCubePSEntry>	entries_;
    ObjectSet<uiSeisMultiCubePSEntry>	selentries_;
    int			curselidx_		= -1;

    uiListBox*		cubefld_		= nullptr;
    uiCheckBox*		allcompfld_		= nullptr;
    uiListBox*		selfld_			= nullptr;
    uiGenInput*		offsfld_		= nullptr;
    uiIOObjSel*		outfld_			= nullptr;
    uiComboBox*		compfld_		= nullptr;

    void		fillEntries();
    void		fillBox(uiListBox*);
    void		recordEntryData();
    void		fullUpdate();
    void		setCompFld(const uiSeisMultiCubePSEntry&);

    void		setInitial(CallBacker*);
    void		inputChg(CallBacker*);
    void		selChg(CallBacker*);
    void		addCube(CallBacker*);
    void		rmCube(CallBacker*);
    bool		acceptOK(CallBacker*) override;
};
