/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.cc,v 1.1 2003-07-16 09:56:21 nanne Exp $
________________________________________________________________________

-*/

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "uimsg.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "emhorizontransl.h"
#include "emhorizon.h"
#include "ptrman.h"
#include "uiexecutor.h"


uiSaveSurfaceDlg::uiSaveSurfaceDlg( uiParent* p, const EM::Horizon& hor_ )
    : uiDialog(p,uiDialog::Setup("Output selection","",""))
    , hor(hor_)
{
    iogrp = new uiIOSurface( this, &hor );
}


bool uiSaveSurfaceDlg::doWrite()
{
    dgbEMHorizonTranslator tr;
    if ( !tr.startWrite(hor) )
        return false;

    EM::SurfaceIODataSelection& sels = tr.selections();
    iogrp->getSelection( sels );

    PtrMan<Executor> exec = tr.writer( *iogrp->selIOObj() );
    uiExecutor dlg( this, *exec );
    bool rv = dlg.execute();

    return rv;
}


bool uiSaveSurfaceDlg::acceptOK( CallBacker* )
{
    return true;
}



uiIOSurfaceDlg::uiIOSurfaceDlg( uiParent* p, CtxtIOObj& c )
    : uiDialog(p,uiDialog::Setup(c.ctxt.forread ? "Input selection"
                                                : "Output selection","",""))
{
    iogrp = new uiIOSurface( this, c );
}


IOObj* uiIOSurfaceDlg::ioObj() const
{
    return iogrp->selIOObj();
}


bool uiIOSurfaceDlg::acceptOK( CallBacker* )
{
    return true;
}

