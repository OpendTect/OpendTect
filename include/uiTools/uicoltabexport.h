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

class uiButtonGroup;
class uiFileInput;
class uiListBox;


mExpClass(uiTools) uiColTabExport final : public uiDialog
{
mODTextTranslationClass(uiColTabExport)
public:

				uiColTabExport(uiParent*);
				~uiColTabExport();

protected:

    uiButtonGroup*		choicefld_;
    uiFileInput*		dirfld_;
    uiListBox*			listfld_;

    void			choiceCB(CallBacker*);
    bool			acceptOK(CallBacker*) override;
    void			fillList();

    void			writeODFile(const ColTab::Sequence&,
					    od_ostream&);
    void			writeAlutFile(const ColTab::Sequence&,
					      od_ostream&);
    void			writeCSVFile(const ColTab::Sequence&,
					     od_ostream&,const char* sep=",");
};
