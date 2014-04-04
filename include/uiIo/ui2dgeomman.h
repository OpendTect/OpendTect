#ifndef ui2dgeomman_h
#define ui2dgeomman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiobjfileman.h"
class uiListBox;

mExpClass(uiIo) ui2DGeomManageDlg : public uiObjFileMan
{
public:

			ui2DGeomManageDlg(uiParent*);
			~ui2DGeomManageDlg();

protected:

    void		manLineGeom(CallBacker*);
    void		lineRemoveCB(CallBacker*);
    void		ownSelChg();
    void		mkFileInfo();
};


#endif

