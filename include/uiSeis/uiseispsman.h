#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.4 2008-09-04 13:31:45 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"


class uiSeisPreStackMan : public uiObjFileMan
{
public:
			uiSeisPreStackMan(uiParent*,bool for2d);
			~uiSeisPreStackMan();

protected:

    void		mkFileInfo();

    void                mergePush(CallBacker*);
    void                mkMultiPush(CallBacker*);
};


#endif
