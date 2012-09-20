#ifndef uiprestacktreeitemmgr_h
#define uiprestacktreeitemmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          November 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "callback.h"

class PSEventsTreeItemFactory;
class uiODMain;

class uiPreStackTreeItemManager : public CallBacker
{
public:
			uiPreStackTreeItemManager(uiODMain&);
			~uiPreStackTreeItemManager();

protected:
    void		surveyChangedCB(CallBacker*);
    
    PSEventsTreeItemFactory*	treeitem_;
    uiODMain&			appl_;
};

#endif
