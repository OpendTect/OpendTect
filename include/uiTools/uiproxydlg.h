#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2012
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "odnetworkaccess.h"

class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiLabeledSpinBox;
class uiLineEdit;

mExpClass(uiTools) uiProxyDlg : public uiDialog
{ mODTextTranslationClass(uiProxyDlg);
public:
			uiProxyDlg(uiParent*);
			~uiProxyDlg();

    void		setHost(const char*);
    const char*		getHost() const;
    void		setPort(int);
    int			getPort() const;

    bool		fillPar(IOPar&,bool) const;
    bool		usePar(const IOPar&);

protected:

    void		initFromSettings();
    bool		saveInSettings();
    void		useProxyCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiCheckBox*		authenticationfld_;
    uiGenInput*		usernamefld_;
    uiLineEdit*		pwdfld_;
    uiLabel*		pwdlabel_;
    uiCheckBox*		savepwdfld_;
    uiGenInput*		useproxyfld_;
    uiGenInput*		hostfld_;
    uiLabeledSpinBox*	portfld_;
};

mExpClass(uiTools) uiNetworkUserQuery : public NetworkUserQuery
{ mODTextTranslationClass(uiNetworkUserQuery);
public:

			uiNetworkUserQuery();

    bool		setFromUser();

    void		setMainWin(uiMainWin* mw)	{ mainwin_ = mw; }

protected:

    uiMainWin*		mainwin_;
};


