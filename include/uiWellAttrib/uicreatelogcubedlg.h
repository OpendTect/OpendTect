#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uidialog.h"
#include "uigroup.h"
#include "multiid.h"

class uiCheckBox;
class uiCreateLogCubeOutputSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiMultiWellLogSel;

mExpClass(uiWellAttrib) uiCreateLogCubeDlg : public uiDialog
{ mODTextTranslationClass(uiCreateLogCubeDlg);
public:
				uiCreateLogCubeDlg(uiParent*,const MultiID*);
				~uiCreateLogCubeDlg();

    MultiID			currentKey() const	{ return key_; }
protected:

    uiMultiWellLogSel*		welllogsel_;
    uiCreateLogCubeOutputSel*	outputgrp_;

    MultiID			key_;
    bool			acceptOK(CallBacker*) override;
};


mExpClass(uiWellAttrib) uiCreateLogCubeOutputSel : public uiGroup
{ mODTextTranslationClass(uiCreateLogCubeOutputSel);
public:

				uiCreateLogCubeOutputSel(uiParent*,
							 bool withwllnm=false);
				~uiCreateLogCubeOutputSel();

    int				getNrRepeatTrcs() const;
    const char*			getPostFix() const;
    bool			withWellName() const;

    void			displayRepeatFld(bool);
    void			setPostFix(const BufferString&);
    void			useWellNameFld(bool yn);

    bool			askOverwrite(const uiString& errmsg) const;

protected:

    uiLabeledSpinBox*		repeatfld_;
    uiGenInput*			savesuffix_;
    uiCheckBox*			savewllnmfld_ = nullptr;

};
