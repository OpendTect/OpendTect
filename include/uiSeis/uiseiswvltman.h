#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseiswvltman.h,v 1.2 2006-10-16 16:45:07 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiCanvas;

class uiSeisWvltMan : public uiObjFileMan
{
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

protected:

    uiCanvas*		wvltfld;

    void		mkFileInfo();

    void		impPush(CallBacker*);
    void		crPush(CallBacker*);
};


#endif
