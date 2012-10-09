#ifndef uiodsysadmcoltabs_h
#define uiodsysadmcoltabs_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H.Bril
 Date:          Jul 2006
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "sets.h"

class uiListBox;


mClass uiODSysAdmColorTabs : public uiDialog
{
public:

			uiODSysAdmColorTabs(uiParent*);
			~uiODSysAdmColorTabs();

protected:

    void		fillList(bool);
    void		rebuildList(int);
    void		addPush(CallBacker*);
    void		rmPush(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiListBox*		listfld;
};


#endif
