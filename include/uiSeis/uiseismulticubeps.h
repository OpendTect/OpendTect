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
class CtxtIOObj;

mExpClass(uiSeis) uiSeisMultiCubePS : public uiDialog
{ mODTextTranslationClass(uiSeisMultiCubePS);

public:
			uiSeisMultiCubePS(uiParent*,const MultiID&);
			~uiSeisMultiCubePS();

    const IOObj*	createdIOObj() const;

protected:

    CtxtIOObj&		ctio_;
    ObjectSet<uiSeisMultiCubePSEntry>	entries_;
    ObjectSet<uiSeisMultiCubePSEntry>	selentries_;
    int			curselidx_;

    uiListBox*		cubefld_;
    uiCheckBox*		allcompfld_;
    uiListBox*		selfld_;
    uiGenInput*		offsfld_;
    uiIOObjSel*		outfld_;
    uiComboBox*		compfld_;

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
