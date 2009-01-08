#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.6 2009-01-08 08:31:03 cvsranojay Exp $
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

    void                mergePush(CallBacker*);
    void                mkMultiPush(CallBacker*);
};


#endif
