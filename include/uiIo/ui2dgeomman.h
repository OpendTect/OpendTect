#ifndef ui2dgeomman_h
#define ui2dgeomman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
 RCS:           $Id: ui2dgeomman.h,v 1.6 2012-08-03 13:00:58 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"
class uiListBox;

mClass(uiIo) ui2DGeomManageDlg : public uiDialog
{
public:

			ui2DGeomManageDlg(uiParent*);
			~ui2DGeomManageDlg();

protected:

    void		lineSetSelCB(CallBacker*);
    void		lineNameSelCB(CallBacker*);
    void		manLineGeom(CallBacker*);

    uiListBox*		linesetfld_;
    uiListBox*		linenamefld_;

    void		removeLineGeom(CallBacker*);
    void		removeLineSetGeom(CallBacker*);
};


#endif

