#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.11 2004-12-06 17:15:04 bert Exp $
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

    uiToolButton*	mrgdmpbut;
    uiToolButton*	cpym2dbut;

    void		ownSelChg();

    void		mergeDump2DPush(CallBacker*);
    void		copyMan2DPush(CallBacker*);

    void		mkFileInfo();
    double		getFileSize(const char*);
};


#endif
