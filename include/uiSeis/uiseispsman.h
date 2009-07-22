#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.8 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"


mClass uiSeisPreStackMan : public uiObjFileMan
{
public:
			uiSeisPreStackMan(uiParent*,bool for2d);
			~uiSeisPreStackMan();

protected:

    bool		is2d_;

    void		mkFileInfo();

    void		copyPush(CallBacker*);
    void                mergePush(CallBacker*);
    void                mkMultiPush(CallBacker*);
};


#endif
