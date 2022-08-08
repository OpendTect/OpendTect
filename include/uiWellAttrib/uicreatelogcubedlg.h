#pragma once
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
 _______________________________________________________________________

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
    uiCheckBox*			savewllnmfld_;

};

