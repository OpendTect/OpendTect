/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/

#include "uimergeseis.h"

#include "bufstringset.h"
#include "seiscbvs.h"
#include "seismerge.h"
#include "seistrctr.h"
#include "ioobjctxt.h"
#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "zdomain.h"

#include "uiioobjselgrp.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseistransf.h"
#include "od_helpids.h"

#include <math.h>


uiMergeSeis::uiMergeSeis( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Seismic file merging"),
				 tr("Specify input/output seismics"),
				 mODHelpKey(mMergeSeisHelpID) ))
{
    IOObjContext ctxt( uiSeisSel::ioContext( Seis::Vol, true ) );
    uiIOObjSelGrp::Setup sgsu( OD::ChooseZeroOrMore );
    sgsu.allowremove( false );
    ctxt.forread_ = true;
    inpfld_ = new uiIOObjSelGrp( this, ctxt, tr("Input Cubes"), sgsu );

    stackfld_ = new uiGenInput( this, tr("Duplicate traces"),
				BoolInpSpec(true,tr("Stack"),tr("Use first")) );
    stackfld_->attach( alignedBelow, inpfld_ );

    uiSeisTransfer::Setup stsu( Seis::Vol );
    stsu.withnullfill( false ).fornewentry( true ).withstep( false );
    transffld_ = new uiSeisTransfer( this, stsu );
    transffld_->attach( alignedBelow, stackfld_ );

    ctxt.forread_ = false;
    outfld_ = new uiSeisSel( this, ctxt, uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, transffld_ );
}


bool uiMergeSeis::acceptOK()
{
    ObjectSet<IOPar> inpars; IOPar outpar;
    if ( !getInput(inpars,outpar) )
	return false;

    SeisMerger mrgr( inpars, outpar, false );
    mrgr.stacktrcs_ = stackfld_->getBoolValue();
    mrgr.setScaler( transffld_->getScaler() );
    uiTaskRunner dlg( this );
    return TaskRunner::execute( &dlg, mrgr );
}


void uiMergeSeis::setInputIds( const DBKeySet& mids )
{
    inpfld_->setChosen( mids );
}


bool uiMergeSeis::getInput( ObjectSet<IOPar>& inpars, IOPar& outpar )
{
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
        return false;

    DBKeySet chosenids;
    inpfld_->getChosen( chosenids );
    if ( chosenids.size()  < 2 )
	{ uiMSG().error( uiStrings::phrSelect(tr("at least 2 inputs")) ); 
								return false;}

    outpar.set( sKey::ID(), outioobj->key() );

    BufferString typestr, zdomstr;
    for ( int idx=0; idx<chosenids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = DBM().get( chosenids[idx] );
	if ( !ioobj )
	    continue;

	if ( !idx )
	{
	    typestr = ioobj->pars().find( sKey::Type() );
	    zdomstr = ioobj->pars().find( ZDomain::sKey() );
	}
	else
	{
	    if ( typestr != ioobj->pars().find(sKey::Type()) )
	    {
		uiMSG().error( tr("Input cubes should be of the same type") );
		return false;
	    }

	    if ( zdomstr != ioobj->pars().find(ZDomain::sKey()) )
	    {
		uiMSG().error( tr("Input cubes should belong to the same"
			          " Z domain") );
		return false;
	    }
	}

	IOPar* iop = new IOPar;
	iop->set( sKey::ID(), chosenids[idx] );
	transffld_->fillPar( *iop );
	inpars += iop;
    }

    if ( typestr.isEmpty() && zdomstr.isEmpty() )
	return true;

    if ( !typestr.isEmpty() ) outioobj->pars().set( sKey::Type(), typestr );
    if ( !zdomstr.isEmpty() ) outioobj->pars().set( ZDomain::sKey(), zdomstr );
    DBM().setEntry( *outioobj );

    return true;
}
