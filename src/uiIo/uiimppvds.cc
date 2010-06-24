/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiimppvds.cc,v 1.1 2010-06-24 15:16:51 cvsbert Exp $";

#include "uiimppvds.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "posvecdatasettr.h"
#include "ioobj.h"
#include "strmprov.h"
#include "datapointset.h"


uiImpPVDS::uiImpPVDS( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import cross-plot data",
				 "Import column data for cross-plots",
				 mTODOHelpID))
{
    uiFileInput::Setup su( uiFileDialog::Txt );
    su.withexamine(true).examstyle(uiFileInput::Setup::Table).forread(true);
    inpfld_ = new uiFileInput( this, "Input file", su );

    haveposfld_ = new uiGenInput( this, "Positions in file",
	    				BoolInpSpec(true) );
    haveposfld_->valuechanged.notify( mCB(this,uiImpPVDS,havePosSel) );
    haveposfld_->attach( alignedBelow, inpfld_ );
    posgrp_ = new uiGroup( this, "POs group" );
    posgrp_->attach( alignedBelow, haveposfld_ );
    posiscoordfld_ = new uiGenInput( posgrp_, "Positions are",
	    				BoolInpSpec(true,"X Y","Inl Crl") );
    havezbox_ = new uiCheckBox( posgrp_, "Z column" );
    havezbox_->attach( rightOf, posiscoordfld_ );
    posgrp_->setHAlignObj( posiscoordfld_ );

    IOObjContext ctxt( mIOObjContext(PosVecDataSet) );
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt, "Output data set" );
    outfld_->attach( alignedBelow, posgrp_ );
}


void uiImpPVDS::havePosSel( CallBacker* )
{
    posgrp_->display( haveposfld_->getBoolValue() );
}


bool uiImpPVDS::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO" );
    return true;
}
