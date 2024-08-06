#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

namespace ColTab { class Sequence; }

class uiFileInput;
class uiGenInput;
class uiListBox;


mExpClass(uiTools) uiColTabExport : public uiDialog
{
mODTextTranslationClass(uiColTabExport)
public:

				uiColTabExport(uiParent*);
				~uiColTabExport();

protected:

    uiGenInput*			choicefld_;
    uiFileInput*		dirfld_;
    uiListBox*			listfld_;

    bool			acceptOK(CallBacker*) override;
    void			fillList();

    void			writeAlutFile(const ColTab::Sequence&,
					      od_ostream&);
    void			writeODFile(const ColTab::Sequence&,
					    od_ostream&);
};
