#ifndef uiproxydlg_h
#define uiproxydlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2012
 RCS:		$Id: uiproxydlg.h,v 1.1 2012-05-25 19:13:10 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiGenInput;
class uiLabeledSpinBox;

mClass uiProxyDlg : public uiDialog
{
public:
			uiProxyDlg(uiParent*);
			~uiProxyDlg();

    void		setHost(const char*);
    const char*		getHost() const;
    void		setPort(int);
    int			getPort() const;

protected:

    void		initFromSettings();
    bool		saveInSettings();
    void		useProxyCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiGenInput*		useproxyfld_;
    uiGenInput*		hostfld_;
    uiLabeledSpinBox*	portfld_;
};

#endif
