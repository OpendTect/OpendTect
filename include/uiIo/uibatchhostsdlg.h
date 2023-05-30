#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    enum		HostLookupMode { StaticIP, NameDNS };
			mDeclareEnumUtils(HostLookupMode)

    enum		Status { Unknown, OK, Error };
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
    BufferString	localaddr_;
    int			prefixlength_		= -1;
    bool		readonly_		= false;

    void		fillTable();
    void		advbutCB(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    void		initUI(CallBacker*);
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
