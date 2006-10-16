#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseiswvltman.h,v 1.1 2006-10-16 14:58:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiToolButton;

class uiSeisWvltMan : public uiObjFileMan
{
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

protected:

    void		mkFileInfo();
};


#endif
