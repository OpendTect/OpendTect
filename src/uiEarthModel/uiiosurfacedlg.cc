/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.cc,v 1.2 2003-07-29 13:03:09 nanne Exp $
________________________________________________________________________

-*/

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "emsurfaceiodata.h"
#include "uimsg.h"
#include "ioobj.h"
#include "emhorizon.h"
#include "emmanager.h"


uiWriteSurfaceDlg::uiWriteSurfaceDlg( uiParent* p, const EM::Horizon& hor_ )
    : uiDialog(p,uiDialog::Setup("Output selection","",""))
    , hor(hor_)
    , auxdataidx(-1)
{
    iogrp = new uiSurfaceOutSel( this, hor );
}


bool uiWriteSurfaceDlg::acceptOK( CallBacker* )
{
    iogrp->processInput();
    if ( auxDataOnly() )
	return checkIfAlreadyPresent();

    return true;
}


void uiWriteSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp->getSelection( sels );
    if ( auxDataOnly() )
    {
	sels.selvalues.erase();
	sels.selvalues += auxdataidx;
    }
}


bool uiWriteSurfaceDlg::auxDataOnly() const
{
    return iogrp->saveAuxDataOnly();
}


bool uiWriteSurfaceDlg::checkIfAlreadyPresent()
{
    BufferString attrnm = iogrp->auxDataName();
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( hor.id(), sd );

    bool present = false;
    auxdataidx = -1;
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
    {
	if ( attrnm == sd.valnames[idx]->buf() )
	{
	    present = true;
	    auxdataidx = idx;
	}
    }

    BufferString msg( "This surface already has an attribute called:\n" );
    msg += attrnm;
    msg += "\nDo you wish to overwrite this data?";

    return !(present && !uiMSG().askGoOn(msg) );
}



uiReadSurfaceDlg::uiReadSurfaceDlg( uiParent* p, const MultiID* emid )
    : uiDialog(p,uiDialog::Setup("Input selection","",""))
{
    if ( emid )
	iogrp = new uiSurfaceAuxSel( this, *emid );
    else
	iogrp = new uiSurfaceSel( this );
}


bool uiReadSurfaceDlg::acceptOK( CallBacker* )
{
    return true;
}


IOObj* uiReadSurfaceDlg::ioObj() const
{
    return iogrp->selIOObj();
}


void uiReadSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp->getSelection( sels );
}
