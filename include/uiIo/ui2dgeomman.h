#ifndef ui2dgeomman_h
#define ui2dgeomman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
 RCS:           $Id: ui2dgeomman.h,v 1.5 2011/04/20 07:02:48 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
class uiListBox;

mClass ui2DGeomManageDlg : public uiDialog
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
