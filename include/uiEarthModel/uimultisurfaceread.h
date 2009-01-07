#ifndef uimultisurfaceread_h
#define uimultisurfaceread_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uimultisurfaceread.h,v 1.6 2009-01-07 06:49:23 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uiiosurface.h"
#include "uidialog.h"

class uiIOObjSelGrp;
class uiDialog;

/*! \brief ui for multiple surface read */

class uiMultiSurfaceRead : public uiIOSurface
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


class uiMultiSurfaceReadDlg : public uiDialog
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
