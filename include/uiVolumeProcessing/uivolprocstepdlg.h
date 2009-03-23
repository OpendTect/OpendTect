#ifndef uivolprocstepdlg_h
#define uivolprocstepdlg_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Mar 2009
 RCS:		$Id: uivolprocstepdlg.h,v 1.1 2009-03-23 11:26:17 cvsbert Exp $
________________________________________________________________________


-*/

#include "uidialog.h"
class uiGenInput;
class uiGroup;


namespace VolProc
{

class Step;

mClass uiStepDialog : public uiDialog
{
public:
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
