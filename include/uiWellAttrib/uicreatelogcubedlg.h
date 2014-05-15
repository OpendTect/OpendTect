#ifndef uicreatelogcubedlg_h
#define uicreatelogcubedlg_h
/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
 RCS:           $Id$
 _______________________________________________________________________

-*/


#include "uiwellattribmod.h"
#include "uidialog.h"
#include "uigroup.h"

class uiCheckBox;
class uiCreateLogCubeOutputSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiMultiWellLogSel;

mExpClass(uiWellAttrib) uiCreateLogCubeDlg : public uiDialog
{
public:
				uiCreateLogCubeDlg(uiParent*,const MultiID*);

protected:

    uiMultiWellLogSel*		welllogsel_;
    uiCreateLogCubeOutputSel*	outputgrp_;

    bool			acceptOK(CallBacker*);
};


mExpClass(uiWellAttrib) uiCreateLogCubeOutputSel : public uiGroup
{
public:

				uiCreateLogCubeOutputSel(uiParent*,
							 bool withmerge);

    const char*			getPostFix() const;
    bool			withWellName() const;
    int				getNrRepeatTrcs() const;

    void			setPostFix(const BufferString&);
    void			useWellNameFld(bool);
    void			displayRepeatFld(bool);

    bool			askOverwrite(BufferString errmsg) const;

protected:

    uiLabeledSpinBox*		repeatfld_;
    uiCheckBox*			savewllnmfld_;
    uiGenInput*			savepostfix_;
    uiCheckBox*			domergefld_;

};

#endif

