#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uisurfaceman.h,v 1.7 2005-02-10 16:23:05 nanne Exp $
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

    void		removeCB(CallBacker*);
    void		renameCB(CallBacker*);
    void		mkFileInfo();
    void		fillAttribList(const BufferStringSet&);
};


#endif
