#ifndef uiiosurfacedlg_h
#define uiiosurfacedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.h,v 1.9 2005-04-06 10:54:24 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOObj;
class uiGenInput;
class uiSurfaceRead;
class uiSurfaceWrite;
class MultiID;

namespace EM { class Surface; class SurfaceIODataSelection; };


/*! \brief Dialog for horizon export */

class uiWriteSurfaceDlg : public uiDialog
{
public:
			uiWriteSurfaceDlg(uiParent*,const EM::Surface&);

    void		getSelection(EM::SurfaceIODataSelection&);
    IOObj*		ioObj() const;

protected:
    uiSurfaceWrite*	iogrp_;
    const EM::Surface&	surface_;

    bool		acceptOK(CallBacker*);
};


class uiReadSurfaceDlg : public uiDialog
{
public:
			uiReadSurfaceDlg(uiParent*,bool ishor);

    IOObj*		ioObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

protected:
    uiSurfaceRead*	iogrp_;
    bool		acceptOK(CallBacker*);
};


class uiStoreAuxData : public uiDialog
{
public:
    			uiStoreAuxData(uiParent*,const EM::Surface&);

    int			getDataFileIdx() const	{ return auxdataidx_; }

protected:
    uiGenInput*		attrnmfld_;
    const EM::Surface&	surface_;

    int			auxdataidx_;
    bool		checkIfAlreadyPresent(const char*);
    bool		acceptOK(CallBacker*);
};

#endif
