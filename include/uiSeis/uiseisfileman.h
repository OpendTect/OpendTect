#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.10 2004-10-28 15:01:35 nanne Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiToolButton;

class uiSeisFileMan : public uiObjFileMan
{
public:
			uiSeisFileMan(uiParent*);
			~uiSeisFileMan();

protected:

    uiToolButton*	mergebut;
    uiToolButton*	copybut;

    void		ownSelChg();

    void		mergePush(CallBacker*);
    void		copyPush(CallBacker*);
    void		man2DPush(CallBacker*);

    void		mkFileInfo();
    double		getFileSize(const char*);
};


#endif
