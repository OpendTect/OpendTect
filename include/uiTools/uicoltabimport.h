#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "uistring.h"

namespace ColTab { class Sequence; class SeqMgr; }

class uiFileInput;
class uiGenInput;
class uiLabel;
class uiListBox;


mExpClass(uiTools) uiColTabImport : public uiDialog
{ mODTextTranslationClass(uiColTabImport)
public:
    enum ImportType	{ OtherUser, ODTColTab, PetrelAlut };
    mDeclareEnumUtils(ImportType)

				uiColTabImport(uiParent*);
				~uiColTabImport();

    const char*			getCurrentSelColTab() const;

protected:

    uiGenInput*			choicefld_;
    uiFileInput*		dirfld_;
    uiGenInput*			dtectusrfld_;
    uiListBox*			listfld_;
    uiLabel*			messagelbl_;

    ObjectSet<ColTab::Sequence> seqs_;

    void			choiceSel(CallBacker*);
    void			usrSel(CallBacker*);
    bool			acceptOK(CallBacker*) override;

    void			showMessage(const uiString&);
    void			showList();

    void			getFromSettingsPar(const IOPar&);
    void			getFromAlutFiles(const BufferStringSet&);

private :
    static uiString		getLabel(ImportType);
};
