#ifndef uiseis2dfrom3d_h
#define uiseis2dfrom3d_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"

class uiSeisSel;
class uiSeis2DSubSel;

mExpClass(uiSeis) uiSeis2DFrom3D : public uiDialog
{
public:
			uiSeis2DFrom3D(uiParent*);
			~uiSeis2DFrom3D();

protected:

    void		cubeSel(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiSeisSel*		data3dfld_;
    uiSeis2DSubSel*	subselfld_;
    uiSeisSel*		data2dfld_;
};

#endif

