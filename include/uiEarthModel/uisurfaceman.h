#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uisurfaceman.h,v 1.6 2004-10-28 15:01:59 nanne Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class BufferStringSet;


class uiSurfaceMan : public uiObjFileMan
{
public:
			uiSurfaceMan(uiParent*,bool);
			~uiSurfaceMan();

protected:

    uiListBox*		attribfld;

    void		remPush(CallBacker*);
    void		mkFileInfo();
    void		fillAttribList(const BufferStringSet&);
};


#endif
