#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2014
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class HostDataList;
class RowCol;
class uiCheckBox;
class uiGenInput;
class uiTable;
class uiToolButton;

mExpClass(uiIo) uiBatchHostsDlg : public uiDialog
{ mODTextTranslationClass(uiBatchHostsDlg)
public:
			enum HostLookupMode { StaticIP, NameDNS };
			mDeclareEnumUtils(HostLookupMode)

			uiBatchHostsDlg(uiParent*);
			~uiBatchHostsDlg();

protected:
    uiTable*		table_;
    uiToolButton*	removebut_;
    uiToolButton*	upbut_;
    uiToolButton*	downbut_;
    uiCheckBox*		autobox_;

    HostDataList&	hostdatalist_;

    void		fillTable();
    void		advbutCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		changedCB(CallBacker*);
    void		addHostCB(CallBacker*);
    void		rmHostCB(CallBacker*);
    void		moveUpCB(CallBacker*);
    void		moveDownCB(CallBacker*);
    void		testHostsCB(CallBacker*);
    void		hostSelCB(CallBacker*);

    void		checkHostData(int row);
    void		ipAddressChanged(int row);
    void		hostNameChanged(int row);
    void		displayNameChanged(int row);
    void		platformChanged(int row);
    void		dataRootChanged(int row);
    void		lookupModeChanged(int row);
};

