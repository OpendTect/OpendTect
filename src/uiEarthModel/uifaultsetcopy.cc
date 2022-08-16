/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2008
________________________________________________________________________

-*/

#include "uifaultsetcopy.h"

#include "uiioobjsel.h"
#include "uiioobjselgrp.h"

#include "ctxtioobj.h"
#include "emfault3d.h"
#include "emfaultset3d.h"
#include "emsurfacetr.h"


uiFault2FaultSet::uiFault2FaultSet( uiParent* p )
    : uiDialog(p,Setup(tr("Copy Faults to FaultSet"),mNoDlgTitle,mTODOHelpKey))
{
    IOObjContext ctxt = mIOObjContext( EMFault3D );
    infld_ = new uiIOObjSelGrp( this, ctxt, tr("Select Faults") );

    IOObjContext fsctxt = mIOObjContext( EMFaultSet3D );
    fsctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, fsctxt, tr("Output FaultSet") );
    outfld_->attach( alignedBelow, infld_ );
}


uiFault2FaultSet::~uiFault2FaultSet()
{
    detachAllNotifiers();
}


void uiFault2FaultSet::setInput( const TypeSet<MultiID>& mids )
{
    infld_->setChosen( mids );
}


MultiID uiFault2FaultSet::key() const
{
    return outfld_->key();
}


bool uiFault2FaultSet::acceptOK( CallBacker* )
{
    return false;
}
