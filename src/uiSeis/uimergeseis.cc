/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimergeseis.h"

#include "bufstringset.h"
#include "seiscbvs.h"
#include "seismerge.h"
#include "seistrctr.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"

#include "uiioobjselgrp.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseisfmtscale.h"
#include "od_helpids.h"

#include <math.h>


uiMergeSeis::uiMergeSeis( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Seismic file merging",
				 "Specify input/output seismics",
				 mODHelpKey(mMergeSeisHelpID) ))
{
    IOObjContext ctxt( uiSeisSel::ioContext( Seis::Vol, true ) );
    uiIOObjSelGrp::Setup sgsu( OD::ChooseZeroOrMore );
    sgsu.allowremove( false );
    ctxt.forread = true;
    inpfld_ = new uiIOObjSelGrp( this, ctxt, "Input Cubes", sgsu );
    inpfld_->selectionChanged.notify( mCB(this,uiMergeSeis,selChangeCB) );

    stackfld_ = new uiGenInput( this, "Duplicate traces",
				BoolInpSpec(true,"Stack","Use first") );
    stackfld_->attach( alignedBelow, inpfld_ );

    scfmtfld_ = new uiSeisFmtScale( this, Seis::Vol, false, false );
    scfmtfld_->attach( alignedBelow, stackfld_ );

    ctxt.forread = false;
    outfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, scfmtfld_ );
}


bool uiMergeSeis::acceptOK( CallBacker* )
{
    ObjectSet<IOPar> inpars; IOPar outpar;
    if ( !getInput(inpars,outpar) )
	return false;

    SeisMerger mrgr( inpars, outpar, false );
    mrgr.stacktrcs_ = stackfld_->getBoolValue();
    mrgr.setScaler( scfmtfld_->getScaler() );
    uiTaskRunner dlg( this );
    return TaskRunner::execute( &dlg, mrgr );
}


void uiMergeSeis::selChangeCB( CallBacker* cb )
{
    const int nrchosen = inpfld_->nrChosen();
    if ( nrchosen < 1 )
	return;

    const MultiID& firstid = inpfld_->chosenID( 0 );
    PtrMan<IOObj> firstobj = IOM().get( firstid );
    if ( firstobj )
	scfmtfld_->updateFrom( *firstobj );
}


bool uiMergeSeis::getInput( ObjectSet<IOPar>& inpars, IOPar& outpar )
{
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
        return false;
    const int inpsz = inpfld_->nrChosen();
    if ( inpsz < 2 )
	{ uiMSG().error( "Please select at least 2 inputs" ); return false;}

    scfmtfld_->updateIOObj( const_cast<IOObj*>(outioobj) );
    outpar.set( sKey::ID(), outioobj->key() );

    for ( int idx=0; idx<inpsz; idx++ )
    {
	IOPar* iop = new IOPar;
	iop->set( sKey::ID(), inpfld_->chosenID(idx) );
	inpars += iop;
    }

    return true;
}
