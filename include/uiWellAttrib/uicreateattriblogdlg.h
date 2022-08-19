#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "bufstringset.h"
#include "createattriblog.h"
#include "welldata.h"

namespace Attrib { class DescSet; }
class NLAModel;
class uiAttrSel;
class uiListBox;
class uiGenInput;
class uiWellExtractParams;

mExpClass(uiWellAttrib) uiCreateAttribLogDlg : public uiDialog
{ mODTextTranslationClass(uiCreateAttribLogDlg);
public:
				uiCreateAttribLogDlg(uiParent*,
						     const BufferStringSet&,
					             const Attrib::DescSet*,
						     const NLAModel*,bool);
				~uiCreateAttribLogDlg();

protected:

    uiAttrSel*			attribfld_;
    uiListBox*			welllistfld_;
    uiGenInput*			lognmfld_;
    uiWellExtractParams*	zrangeselfld_;
    const BufferStringSet&	wellnames_;
    int				sellogidx_;
    bool			singlewell_;
    AttribLogCreator::Setup	datasetup_;

    bool			inputsOK(Well::Data&);
    bool			acceptOK(CallBacker*) override;
    void			init(CallBacker*);
    void			selDone(CallBacker*);
};
