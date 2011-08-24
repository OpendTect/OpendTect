#ifndef uivolprocstepdlg_h
#define uivolprocstepdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2009
 RCS:		$Id: uivolprocstepdlg.h,v 1.3 2011-08-24 13:19:43 cvskris Exp $
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
