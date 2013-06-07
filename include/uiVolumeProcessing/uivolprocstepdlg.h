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

#include "uidialog.h"
#include "factory.h"


class uiGenInput;
class uiGroup;

namespace VolProc
{

class Step;

mClass uiStepDialog : public uiDialog
{
public:
    mDefineFactory2ParamInClass(uiStepDialog,uiParent*,Step*,factory);

    			uiStepDialog(uiParent*,const char* stepnm,Step*);
    virtual bool	isOK() const		{ return true; }
    bool		acceptOK(CallBacker*);

protected:

    uiGenInput*		namefld_;
    Step*		step_;

    void		addNameFld(uiObject* alignobj);
    void		addNameFld(uiGroup* aligngrp);
    friend class	uiChain;

};


}; //namespace

#endif
