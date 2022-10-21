#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "uicompoundparsel.h"
#include "uieditobjectlist.h"

mExpClass(uiTools) uiPathListEditor : public uiEditObjectList
{  mODTextTranslationClass(uiPathListEditor);
public:
			uiPathListEditor(uiParent*,const BufferStringSet&);
			~uiPathListEditor();

    void		fillList(int);
    void		editReq(bool) override;
    void		removeReq() override;
    void		itemSwitch(bool) override;

    BufferStringSet	paths_;
};


mExpClass(uiTools) uiPathSel : public uiCheckedCompoundParSel
{ mODTextTranslationClass(uiPathSel);
public:
			uiPathSel(uiParent*,const uiString&);
			~uiPathSel();

    void		setPaths(const BufferStringSet&);
    BufferString	getSummary() const override;
    BufferStringSet	getPaths() const { return paths_; }

    void		setEmpty();

    Notifier<uiPathSel> selChange;

protected:
    void		checkedCB(CallBacker*);
    void		doDlg(CallBacker*);
    void		initGrp(CallBacker*);

    BufferStringSet	paths_;
};
