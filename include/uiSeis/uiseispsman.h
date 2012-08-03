#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.12 2012-08-03 13:01:08 cvskris Exp $
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiobjfileman.h"


mClass(uiSeis) uiSeisPreStackMan : public uiObjFileMan
{
public:
			uiSeisPreStackMan(uiParent*,bool for2d);
			~uiSeisPreStackMan();

    mDeclInstanceCreatedNotifierAccess(uiSeisPreStackMan);

protected:

    bool		is2d_;

    void		mkFileInfo();

    void		copyPush(CallBacker*);
    void                mergePush(CallBacker*);
    void                mkMultiPush(CallBacker*);
};


#endif

