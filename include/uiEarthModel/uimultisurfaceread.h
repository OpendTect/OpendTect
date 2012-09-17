#ifndef uimultisurfaceread_h
#define uimultisurfaceread_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uimultisurfaceread.h,v 1.8 2009/07/22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiiosurface.h"
#include "uidialog.h"

class uiIOObjSelGrp;
class uiDialog;

/*! \brief ui for multiple surface read */

mClass uiMultiSurfaceRead : public uiIOSurface
{
public:
			uiMultiSurfaceRead(uiParent*,const char* type);
			~uiMultiSurfaceRead();

    uiIOObjSelGrp*	objselGrp()		{ return ioobjselgrp_;}

    void		getSurfaceIds(TypeSet<MultiID>&) const;
    void		getSurfaceSelection(EM::SurfaceIODataSelection&) const;

    Notifier<uiMultiSurfaceRead>	singleSurfaceSelected;

protected:

    uiIOObjSelGrp*	ioobjselgrp_;

    void                dClck(CallBacker*);
    void		selCB(CallBacker*);
};


mClass uiMultiSurfaceReadDlg : public uiDialog
{
public:
			uiMultiSurfaceReadDlg(uiParent*,const char* type);

    uiMultiSurfaceRead*	iogrp()		{ return surfacefld_; }

protected:

    void		statusMsg(CallBacker*);
    bool		acceptOK(CallBacker*);

    uiMultiSurfaceRead*	surfacefld_;
};

#endif
