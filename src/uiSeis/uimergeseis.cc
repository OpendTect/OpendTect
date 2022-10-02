/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimergeseis.h"

#include "bufstringset.h"
#include "seiscbvs.h"
#include "seisioobjinfo.h"
#include "seismerge.h"
#include "seistrctr.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "zdomain.h"

#include "uiioobjselgrp.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
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


uiMergeSeis::~uiMergeSeis()
{}


bool uiMergeSeis::acceptOK( CallBacker* )
{
    ObjectSet<IOPar> inpars;
    IOPar outpar;
    if ( !getInput(inpars,outpar) )
	return false;

    SeisMerger mrgr( inpars, outpar, false );
    mrgr.stacktrcs_ = stackfld_->getBoolValue();
    mrgr.setScaler( transffld_->getScaler() );
    uiTaskRunner dlg( this );
    return TaskRunner::execute( &dlg, mrgr );
}


void uiMergeSeis::setInputIds( const TypeSet<MultiID>& mids )
{
    inpfld_->setChosen( mids );
}


bool uiMergeSeis::getInput( ObjectSet<IOPar>& inpars, IOPar& outpar )
{
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
        return false;

    TypeSet<MultiID> chosenids;
    inpfld_->getChosen( chosenids );
    if ( chosenids.size()  < 2 )
	{ uiMSG().error( tr("Please select at least 2 inputs") ); return false;}

    outpar.set( sKey::ID(), outioobj->key() );

    BufferString typestr, zdomstr;
    for ( int idx=0; idx<chosenids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( chosenids[idx] );
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
	    if ( !SeisIOObjInfo::isCompatibleType(typestr,curtypestr) )
	    {
		uiMSG().error( tr("Input cubes are of incompatible types") );
		return false;
	    }
	    else if ( &ZDomain::Def::get(zdomstr)
		    != &ZDomain::Def::get(curzdomstr) )
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

    uiSeisIOObjInfo ioobjinfo( *outioobj, true );
    SeisIOObjInfo::SpaceInfo spi( transffld_->spaceInfo() );
    if ( !ioobjinfo.checkSpaceLeft(spi) )
	return false;

    if ( typestr.isEmpty() && zdomstr.isEmpty() )
	return true;

    outioobj->pars().update( sKey::Type(), typestr );
    outioobj->pars().update( ZDomain::sKey(), zdomstr );
    IOM().commitChanges( *outioobj );

    return true;
}
