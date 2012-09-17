/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimergeseis.cc,v 1.46 2009/08/21 10:11:46 cvsbert Exp $";

#include "uimergeseis.h"

#include "bufstringset.h"
#include "seismerge.h"
#include "seistrctr.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"

#include "uiseissel.h"
#include "uilistbox.h"
#include "uitaskrunner.h"
#include "uigeninput.h"
#include "uimsg.h"

#include <math.h>


uiMergeSeis::uiMergeSeis( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Seismic file merging",
				 "Specify input/output seismics",
				 "103.1.2"))
    , ctio_(*mMkCtxtIOObj(SeisTrc))
{
    IOM().to( ctio_.ctxt.getSelKey() );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    BufferStringSet ioobjnms;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	if ( ioobjs[idx]->isReadDefault() )
	    ioobjnms.add( ioobjs[idx]->name() );
    }

    ioobjnms.sort();
    for ( int idx=0; idx<ioobjnms.size(); idx++ )
    {
	const char* nm = ioobjnms.get(idx).buf();
	const IOObj* ioobj = (*IOM().dirPtr())[nm];
        ioobjids_ += new MultiID( ioobj ? (const char*)ioobj->key() : "" );
    }
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Input Cubes", true );
    inpfld_ = llb->box();
    inpfld_->addItems( ioobjnms );
    inpfld_->setCurrentItem( 0 );

    stackfld_ = new uiGenInput( this, "Duplicate traces",
	    			BoolInpSpec(true,"Stack","Use first") );
    stackfld_->attach( alignedBelow, llb );

    ctio_.ctxt.forread = false;
    outfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, stackfld_ );
}


uiMergeSeis::~uiMergeSeis()
{
    delete ctio_.ioobj; delete &ctio_;
    deepErase( ioobjids_ );
}


bool uiMergeSeis::acceptOK( CallBacker* )
{
    ObjectSet<IOPar> inpars; IOPar outpar;
    if ( !getInput(inpars,outpar) )
	return false;

    SeisMerger mrgr( inpars, outpar, false );
    mrgr.stacktrcs_ = stackfld_->getBoolValue();
    uiTaskRunner dlg( this );
    return dlg.execute( mrgr );
}


bool uiMergeSeis::getInput( ObjectSet<IOPar>& inpars, IOPar& outpar )
{
    if ( !outfld_->commitInput() )
    {
	if ( outfld_->isEmpty() )
	    uiMSG().error( "Please enter an output Seismic data set name" );
        return false;
    }
    outpar.set( sKey::ID, ctio_.ioobj->key() );

    ObjectSet<IOObj> selobjs;
    for ( int idx=0; idx<inpfld_->size(); idx++ )
    {
        if ( inpfld_->isSelected(idx) )
            selobjs += IOM().get( *ioobjids_[idx] );
    }
    const int inpsz = selobjs.size();
    if ( inpsz < 2 )
    {
	uiMSG().error( "Please select at least 2 inputs" ); 
	return false; 
    }

    static const char* optdirkey = "Optimized direction";
    BufferString type = "";
    BufferString optdir = "";
    for ( int idx=0; idx<inpsz; idx++ )
    {
	const IOObj& ioobj = *selobjs[idx];
	if ( idx == 0 ) 
	{
	    if ( ioobj.pars().hasKey(sKey::Type) )
		type = ioobj.pars().find(sKey::Type);
	    if ( ioobj.pars().hasKey(optdirkey) )
		optdir = ioobj.pars().find( optdirkey );
	}
	IOPar* iop = new IOPar;
	iop->set( sKey::ID, ioobj.key() );
	inpars += iop;
    }
    deepErase( selobjs );

    if ( type.isEmpty() )
	ctio_.ioobj->pars().removeWithKey( sKey::Type );
    else
	ctio_.ioobj->pars().set( sKey::Type, type );
    if ( optdir.isEmpty() )
	ctio_.ioobj->pars().removeWithKey( optdirkey );
    else
	ctio_.ioobj->pars().set( optdirkey, optdir );
    IOM().commitChanges( *ctio_.ioobj );

    return true;
}
