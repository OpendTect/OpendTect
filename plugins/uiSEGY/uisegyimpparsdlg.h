#ifndef uisegyimpparsdlg_h
#define uisegyimpparsdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2015
 RCS:		$Id:$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "typeset.h"
#include "uidialog.h"

class uiListBox;
class uiCheckBox;
class uiGenInput;


mExpClass(uiSEGY) uiSEGYReadImpParsDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYReadImpParsDlg)
public:

			uiSEGYReadImpParsDlg(uiParent*,const char* defnm=0);

    const IOPar&	pars() const		{ return iop_; }
    const char*		parName() const		{ return parname_; }

protected:

    uiListBox*		listfld_;
    uiCheckBox*		asdefbox_;

    mutable BufferString parname_;
    mutable IOPar	iop_;

    bool		acceptOK(CallBacker*);

};


mExpClass(uiSEGY) uiSEGYStoreImpParsDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYStoreImpParsDlg)
public:

			uiSEGYStoreImpParsDlg(uiParent*,const IOPar&,
						const char* defnm=0);

    const char*		parName() const		{ return parname_; }

protected:

    uiListBox*		listfld_;
    uiGenInput*		namefld_;

    mutable BufferString parname_;

    bool		acceptOK(CallBacker*);

};



#endif
