#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.18 2009-07-22 16:01:22 cvsbert Exp $
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
    const char*		getDefKey() const;

    void		mkFileInfo();
    double		getFileSize(const char*,int&) const;
};


#endif
