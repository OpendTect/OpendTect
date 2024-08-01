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


mExpClass(uiTools) uiColTabExport : public uiDialog
{ mODTextTranslationClass(uiColTabExport)
public:
    enum ExportType	{ OtherUser, ODTColTab, PetrelAlut };
    mDeclareEnumUtils(ExportType)

				uiColTabExport(uiParent*);
				~uiColTabExport();

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
    static uiString		getLabel(ExportType);
};
