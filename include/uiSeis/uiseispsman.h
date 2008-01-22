#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.3 2008-01-22 15:04:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiToolButton;

class uiSeisPreStackMan : public uiObjFileMan
{
public:
			uiSeisPreStackMan(uiParent*,bool for2d);
			~uiSeisPreStackMan();

protected:

    void		mkFileInfo();

    uiToolButton*       mrgbut;
    void                mergePush(CallBacker*);
};


#endif
