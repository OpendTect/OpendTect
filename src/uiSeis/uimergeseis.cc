/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:		$Id: uimergeseis.cc,v 1.5 2002-05-07 16:04:18 nanne Exp $
________________________________________________________________________

-*/

#include "uimergeseis.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "seissingtrcproc.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uidset.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "binidselimpl.h"
#include "sorting.h"
#include "uiexecutor.h"

#include <math.h>


uiMergeSeis::uiMergeSeis( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Seismic file merging",
				     "Specify input/output seismics",
				     "0"))
	, ctio(SeisTrcTranslator::ioContext())
	, req(0)
	, inpsz(0)
	, rev(false)
	, proc(0)
{
    IOM().to( ctio.ctxt.stdSelKey() );
    const UserIDObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    UserIDSet ioobjnms("Stored seismic data");
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
        const IOObj& ioobj = *ioobjs[idx];
        if ( strcmp(ioobj.translator(),"CBVS") ) continue;
        ioobjnms.add( (const char*)ioobj.name() );
        ioobjids += new MultiID( ioobj.key() );
    }

    if ( ioobjnms.size() )
    {
	ioobjnms.sort();
	ioobjnms.setCurrent(0);
    }
    seisinpfld = new uiLabeledListBox( this, "Select Seismics to merge", true );
    seisinpfld->box()->addItems( ioobjnms );

    ctio.ctxt.forread = false;
    ctio.ctxt.trglobexpr = "CBVS";
    ctio.ctxt.deftransl = "CBVS";
    seisoutfld = new uiIOObjSel( this, ctio, "Output to", false );
    seisoutfld->attach( alignedBelow, seisinpfld );

    remfld = new uiCheckBox( this, "Remove original files on succes" );
    remfld->attach( alignedBelow, seisoutfld );
    remfld->setChecked( false );
    remfld->display( false ); // TODO: check if merge is really succesfull
}


uiMergeSeis::~uiMergeSeis()
{
    delete proc;
}


bool uiMergeSeis::acceptOK( CallBacker* )
{
    if ( !seisoutfld->commitInput(true) )
    {
        uiMSG().error( "Please enter an output Seismic data set name" );
        return false;
    }

    if ( remfld->isChecked() && 
	 !uiMSG().askGoOn( "Original files will be deleted\n"
	        	   "Do you wish to continue?" ) )
	return false;

    if ( !handleInput() ) return false;
    if ( !checkRanges() ) return false;

    const char* txt = "File merger";
    proc = new SeisSingleTraceProc( selobjs, ctio.ioobj, txt, &seliops );
    uiExecutor dlg( this, *proc );
    dlg.go();

    if ( remfld->isChecked() )
    {
	for ( int idx=0; idx<inpsz; idx++ )
	{
	    IOObj* ioobj = selobjs[idx];
	    if ( !ioobj->implRemove() ) 
	    {
		BufferString msg( "Could not remove\n" );
		msg += ioobj->fullUserExpr(YES);
		uiMSG().error( msg );
		continue;
	    }

	    IOM().removeAux( ioobj->key() );
	    IOM().dirPtr()->permRemove( ioobj->key() );
	}
    }

    return true;
}


bool uiMergeSeis::handleInput()
{
    selobjs.erase();
    seliops.erase();
    for ( int idx=0; idx<seisinpfld->box()->size(); idx++ )
    {
        if ( seisinpfld->box()->isSelected(idx) )
            selobjs += IOM().get( *ioobjids[idx] );
    }

    inpsz = selobjs.size();
    if ( !inpsz )
    {
	uiMSG().error( "Please select input" ); 
	return false; 
    }

    int order[inpsz];
    int inlstart[inpsz];
    BinIDSampler bs;
    StepInterval<float> zrg;
    StepInterval<float> zrgprev;
    BufferString type = "";
    for ( int idx=0; idx<inpsz; idx++ )
    {
	IOObj* ioobj = selobjs[idx];
	if ( ioobj->pars().size() && ioobj->pars().hasKey("Type") )
	{    
	    if ( !idx ) 
		type = ioobj->pars().find("Type");
	    else if ( type != ioobj->pars().find("Type") )
	    {
		uiMSG().error( "Different file types selected" );
		return false;
	    }
	}

        if ( !SeisTrcTranslator::getRanges( *ioobj, bs, zrg ) )
	{
	    BufferString msg( "Cannot read \"" );
	    msg += ioobj->name(); msg += "\"";
	    uiMSG().error( msg );
	    return false;
	}

	IOPar iopar;
	iopar.set( BinIDSelector::sKeyseltyp, BinIDSelector::sKeyseltyps[2] );
	iopar.set( BinIDSelector::sKeyfinl, bs.start.inl );
        iopar.set( BinIDSelector::sKeylinl, bs.stop.inl );
        iopar.set( BinIDSelector::sKeystepinl, abs(bs.step.inl) );
        iopar.set( BinIDSelector::sKeyfcrl, bs.start.crl );
        iopar.set( BinIDSelector::sKeylcrl, bs.stop.crl );
        iopar.set( BinIDSelector::sKeystepcrl, abs(bs.step.crl) );
	seliops += new IOPar( iopar );
	order[idx] = idx;
	inlstart[idx] = bs.start.inl;
	if ( bs.step.inl < 0 ) 
        { 
	    inlstart[idx] *= -1; 
	    rev = true; 
	}

	if ( !idx ) zrgprev = zrg;
	if ( zrg != zrgprev )
	{
	    uiMSG().error( "Z-range is not equal,\n"
			   "filemerging will not continue." );
	    return false;
	}
    }

    ctio.ioobj->pars().set( "Type", type );
    IOM().dirPtr()->commitChanges( ctio.ioobj );

    sort_coupled( inlstart, order, inpsz );

    ObjectSet<IOObj> objs;
    for ( int idx=0; idx<inpsz; idx++ )
	objs += selobjs[idx]->clone();
    deepErase( selobjs );
    ObjectSet<IOPar> iops; deepCopy( iops, seliops ); deepErase( seliops );
    for ( int idx=0; idx<inpsz; idx++ )
    {
	selobjs += objs[ order[idx] ];
	seliops += iops[ order[idx] ];
    } 

    return true;
}


bool uiMergeSeis::checkRanges()
{
    int inlstop; int inlstart; int step;
    int a = rev ? 1 : 0; int b = rev ? 0 : 1;
    for ( int idx=1; idx<inpsz; idx++ )
    {
	seliops[idx-a]->get( BinIDSelector::sKeystepinl, step );
	seliops[idx-a]->get( BinIDSelector::sKeyfinl, inlstart );
	seliops[idx-b]->get( BinIDSelector::sKeylinl, inlstop );
	if ( inlstop >= inlstart )
        {
	   BufferString msg( "Overlap found in data\n" );
	   msg += "Do you wish to continue?";
	   if ( !uiMSG().askGoOn( msg ) ) return false;
	   seliops[idx-a]->set( BinIDSelector::sKeyfinl, inlstop + step );
	}
    }
 
    return true;
}
