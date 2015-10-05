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
namespace Repos { class IOParSet; class IOPar; }


mExpClass(uiSEGY) uiSEGYImpParsDlg : public uiDialog
{ mODTextTranslationClass(uiSEGYImpParsDlg)
public:

			uiSEGYImpParsDlg(uiParent*,bool,const char*);
			~uiSEGYImpParsDlg();

    const char*		parName() const		{ return parname_; }

protected:

    uiListBox*		listfld_;
    Repos::IOParSet&	parset_;

    mutable BufferString parname_;

    bool		acceptOK(CallBacker*);
    virtual bool	doIO()			= 0;

};


mExpClass(uiSEGY) uiSEGYReadImpParsDlg : public uiSEGYImpParsDlg
{ mODTextTranslationClass(uiSEGYReadImpParsDlg)
public:

			uiSEGYReadImpParsDlg(uiParent*,const char* defnm=0);

    const IOPar*	pars() const;

protected:

    virtual bool	doIO();

};


mExpClass(uiSEGY) uiSEGYStoreImpParsDlg : public uiSEGYImpParsDlg
{ mODTextTranslationClass(uiSEGYStoreImpParsDlg)
public:

			uiSEGYStoreImpParsDlg(uiParent*,const IOPar&,
						const char* defnm=0);

protected:

    uiGenInput*		namefld_;
    uiCheckBox*		asdefbox_;

    virtual bool	doIO();

};



#endif
