#ifndef uivolprocstepdlg_h
#define uivolprocstepdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2009
 RCS:		$Id$
________________________________________________________________________


-*/

#include "uivolumeprocessingmod.h"
#include "uidialog.h"
#include "factory.h"

class BufferStringSet;
class uiGenInput;
class uiGroup;
class uiTable;

namespace VolProc
{

class Step;

mExpClass(uiVolumeProcessing) uiStepDialog : public uiDialog
{
public:
    mDefineFactory2ParamInClass(uiStepDialog,uiParent*,Step*,factory);

			uiStepDialog(uiParent*,const char* stepnm,Step*);
    virtual bool	isOK() const		{ return true; }

protected:

    uiTable*		multiinpfld_;
    uiGenInput*		namefld_;
    Step*		step_;

    void		addMultiInputFld();
    void		initInputTable(int nrinputs);
    void		getStepNames(BufferStringSet&) const;
    void		addNameFld(uiObject* alignobj,bool leftal=false);
    void		addNameFld(uiGroup* aligngrp,bool leftal=false);
    friend class	uiChain;

    void		addConnectionFromMultiInput();
    void		addDefaultConnection();
    bool		acceptOK(CallBacker*);
};

} // namespace VolProc

#endif

