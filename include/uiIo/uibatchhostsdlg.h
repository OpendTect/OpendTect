#ifndef uibatchhostsdlg_h
#define uibatchhostsdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class HostDataList;
class RowCol;
class uiGenInput;
class uiTable;
class uiToolButton;

mExpClass(uiIo) uiBatchHostsDlg : public uiDialog
{ mODTextTranslationClass(uiBatchHostsDlg)
public:
			uiBatchHostsDlg(uiParent*);
			~uiBatchHostsDlg();

protected:
    uiTable*		table_;
    uiToolButton*	upbut_;
    uiToolButton*	downbut_;

    HostDataList&	hostdatalist_;

    void		fillTable();
    bool		fillHostData(const RowCol&);
    void		advbutCB(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		changedCB(CallBacker*);
    void		addHostCB(CallBacker*);
    void		rmHostCB(CallBacker*);
    void		moveUpCB(CallBacker*);
    void		moveDownCB(CallBacker*);
    void		testHostsCB(CallBacker*);
    void		hostSelCB(CallBacker*);
};

#endif

