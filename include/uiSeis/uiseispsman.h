#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.5 2008-12-15 13:46:28 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"


class uiSeisPreStackMan : public uiObjFileMan
{
public:
			uiSeisPreStackMan(uiParent*,bool for2d);
			~uiSeisPreStackMan();

protected:

    bool		is2d_;

    void		mkFileInfo();

    void                mergePush(CallBacker*);
    void                mkMultiPush(CallBacker*);
};


#endif
