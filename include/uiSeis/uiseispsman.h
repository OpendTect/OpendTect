#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.2 2007-11-01 07:10:34 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiToolButton;

class uiSeisPreStackMan : public uiObjFileMan
{
public:
			uiSeisPreStackMan(uiParent*);
			~uiSeisPreStackMan();

protected:

    void		mkFileInfo();

    uiToolButton*       mrgbut;
    void                mergePush(CallBacker*);
};


#endif
