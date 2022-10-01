#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uidialog.h"
#include "factory.h"
#include "uistring.h"

class BufferStringSet;
class uiGenInput;
class uiGroup;
class uiTable;

namespace VolProc
{

class Step;

mExpClass(uiVolumeProcessing) uiStepDialog : public uiDialog
{ mODTextTranslationClass(uiStepDialog);
public:
    mDefineFactory3ParamInClass(uiStepDialog,uiParent*,Step*,bool,factory);

			uiStepDialog(uiParent*,const uiString&,Step*,
				     bool is2d=false);
			~uiStepDialog();

    virtual bool	isOK() const		{ return true; }

protected:

    uiTable*		multiinpfld_;
    uiGenInput*		namefld_;
    Step*		step_;
    bool		is2d_;

    void		addMultiInputFld();
    void		addMultiInputFld(uiGroup*);
    void		initInputTable(int nrinputs);
    void		setInputsFromWeb();
    void		getStepNames(BufferStringSet&) const;
    void		addNameFld(uiObject* alignobj,bool leftal=false);
    void		addNameFld(uiGroup* aligngrp,bool leftal=false);
    friend class	uiChain;

    void		addConnectionFromMultiInput();
    void		addDefaultConnection();
    bool		acceptOK(CallBacker*) override;
};

} // namespace VolProc
