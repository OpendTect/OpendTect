#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.1 2007-08-29 09:52:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"


class uiSeisPreStackMan : public uiObjFileMan
{
public:
			uiSeisPreStackMan(uiParent*);
			~uiSeisPreStackMan();

protected:

    void		mkFileInfo();

};


#endif
