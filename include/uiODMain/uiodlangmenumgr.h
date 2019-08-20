#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		Nov 2017
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "callback.h"
#include "uistring.h"
#include "uidialog.h"
#include "uibutton.h"
#include "uicombobox.h"

class uiMenu;
class uiODMenuMgr;
class uiLanguageSel;

/*!\brief The OpendTect language menu manager */

mExpClass( uiODMain ) uiFetchLatestQMDlg : public uiDialog
{ mODTextTranslationClass( uiFetchLatestQMDlg )
public:

			uiFetchLatestQMDlg( uiParent* );
			~uiFetchLatestQMDlg();

    BufferString getFileName() { return fnm_; }
    BufferString getLocaleName() { return localenm_; }
    bool         changeToLatestQM()
					{
 return makecurrtransl_->isChecked();
}

protected:

    uiLabeledComboBox*	    fetchlanglist_;
    uiCheckBox*		    makecurrtransl_;
    bool		    acceptOK();
    void		    getUsersDetails();
    void		    generateQMLink();
    BufferString	    fnm_;
    BufferString	    localenm_;
    void		    getUserDetails(BufferString&);
};


mExpClass(uiODMain) uiODLangMenuMgr : public CallBacker
{ mODTextTranslationClass(uiODLangMenuMgr)
public:
			uiODLangMenuMgr(uiODMenuMgr&);
			~uiODLangMenuMgr();

protected:

    void		setLanguageMenu();

    void		languageChangeCB(CallBacker*);
    void		languageSelectedCB(CallBacker*);
    void		fetchLatestTranslationCB( CallBacker* );

    uiODMenuMgr&	mnumgr_;
    uiMenu*		langmnu_;
    uiLanguageSel*	selfld_;

};
