#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uisurfaceman.h,v 1.10 2006-06-01 10:37:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
class uiListBox;
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
    void		copyCB(CallBacker*);
    void		setRelations(CallBacker*);

    void		mkFileInfo();
    void		fillAttribList(const BufferStringSet&);
};


#endif
