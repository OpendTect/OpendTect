#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.16 2009-03-19 09:01:55 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
class uiToolButton;


mClass uiSeisFileMan : public uiObjFileMan
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
    void		manPS3D(CallBacker*);
    void		manPS2D(CallBacker*);
    void		makeDefault(CallBacker*);

    void		mkFileInfo();
    double		getFileSize(const char*,int&) const;
};


#endif
