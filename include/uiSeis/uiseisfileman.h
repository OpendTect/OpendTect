#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.13 2007-10-15 15:26:26 cvsbert Exp $
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
    uiToolButton*	browsebut;

    void		ownSelChg();

    void		mergeDump2DPush(CallBacker*);
    void		browsePush(CallBacker*);
    void		copyMan2DPush(CallBacker*);
    void		manPS(CallBacker*);

    void		mkFileInfo();
    double		getFileSize(const char*,int&) const;
};


#endif
