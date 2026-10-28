#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uidialog.h"

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

    static void		initClass();

protected:

    void		initFromSettings();
    bool		saveInSettings();
    void		useProxyCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiCheckBox*		authenticationfld_;
    uiGenInput*		usernamefld_;
    uiLineEdit*		pwdfld_;
    uiLabel*		pwdlabel_;
    uiCheckBox*		savepwdfld_;
    uiGenInput*		useproxyfld_;
    uiGenInput*		hostfld_;
    uiLabeledSpinBox*	portfld_;
};

