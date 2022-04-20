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
#include "uistringset.h"

class HostDataList;
class RowCol;
class uiCheckBox;
class uiGenInput;
class uiTable;
class uiToolButton;

mExpClass(uiIo) uiBatchHostsDlg : public uiDialog
{ mODTextTranslationClass(uiBatchHostsDlg)
public:
			enum HostLookupMode {StaticIP,NameDNS};
			mDeclareEnumUtils(HostLookupMode)

			enum Status {Unknown,OK,Error};
			mDeclareEnumUtils(Status)

			uiBatchHostsDlg(uiParent*);
			~uiBatchHostsDlg();

protected:
    uiTable*		table_;
    uiToolButton*	removebut_;
    uiToolButton*	upbut_;
    uiToolButton*	downbut_;
    uiCheckBox*		autohostbox_;
    uiCheckBox*		autoinfobox_;

    HostDataList&	hostdatalist_;
    TypeSet<Status>	hoststatus_;

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

    uiRetVal		doStatusPacket(int row,const char*,const IOPar&);
    void		checkHostData(int row);
    void		ipAddressChanged(int row);
    void		hostNameChanged(int row);
    void		displayNameChanged(int row);
    void		platformChanged(int row);
    void		dataRootChanged(int row);
    void		lookupModeChanged(int row);
};

