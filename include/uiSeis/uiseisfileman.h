#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.12 2007-08-29 09:52:23 cvsbert Exp $
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
    void		manPS(CallBacker*);

    void		mkFileInfo();
    double		getFileSize(const char*,int&) const;
};


#endif
