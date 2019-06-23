/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/

#include "uimergeseis.h"

#include "seiscbvs.h"
#include "seismerge.h"
#include "seistrctr.h"
#include "ioobjctxt.h"
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
		BoolInpSpec(true,uiStrings::sStack(),uiStrings::sUseFirst()) );
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

    SeisMerger mrgr( inpars, outpar, stackfld_->getBoolValue(),
		     Seis::MultiProvider::Maximum );
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
    bool icompattypefound = false;
    bool icompatzdomfound = false;

    BufferString typestr, zdomstr;
    for ( int idx=0; idx<chosenids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = chosenids[idx].getIOObj();
	if ( !ioobj )
	    continue;

	const BufferString curtypestr( ioobj->pars().find( sKey::Type() ) );
	const BufferString curzdomstr( ioobj->pars().find( ZDomain::sKey() ) );
	if ( !idx )
	{
	    typestr = curtypestr;
	    zdomstr = curzdomstr;
	}
	else
	{
	    if ( !SeisIOObjInfo::isCompatibleType( typestr, curtypestr ) )
		icompattypefound = true;
	    if ( &ZDomain::Def::get(zdomstr) != &ZDomain::Def::get(curzdomstr) )
		icompatzdomfound = true;
	}

	IOPar* iop = new IOPar;
	iop->set( sKey::ID(), chosenids[idx] );
	transffld_->fillPar( *iop );
	inpars += iop;
    }

    if ( icompattypefound || icompatzdomfound )
    {
	if ( !uiMSG().askGoOn( tr("Input cubes have incompatible type."
				  "\nAre you sure you want to continue?") ) )
	    return false;
    }
    else if ( !typestr.isEmpty() || !zdomstr.isEmpty() )
    {
	outioobj->pars().update( sKey::Type(), typestr );
	outioobj->pars().update( ZDomain::sKey(), zdomstr );
	outioobj->commitChanges();
    }

    return true;
}
