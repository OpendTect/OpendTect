/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:		$Id: uimergeseis.cc,v 1.19 2004-06-28 16:00:05 bert Exp $
________________________________________________________________________

-*/

#include "uimergeseis.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "iopar.h"
#include "seissingtrcproc.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "uiexecutor.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "binidselimpl.h"
#include "sorting.h"
#include "uiexecutor.h"
#include "uiseistransf.h"
#include "keystrs.h"
#include "survinfo.h"
#include "bufstringset.h"

#include <math.h>


uiMergeSeis::uiMergeSeis( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Seismic file merging",
				     "Specify input/output seismics",
				     "103.1.2"))
	, ctio(*mMkCtxtIOObj(SeisTrc))
	, req(0)
	, inpsz(0)
	, rev(false)
	, proc(0)
{
    IOM().to( ctio.ctxt.stdSelKey() );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();
    BufferStringSet ioobjnms("Stored seismic data");
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
        const IOObj& ioobj = *ioobjs[idx];
        if ( strcmp(ioobj.translator(),"CBVS") ) continue;
        ioobjnms.add( ioobj.name() );
    }

    ioobjnms.sort();
    for ( int idx=0; idx<ioobjnms.size(); idx++ )
    {
	const char* nm = ioobjnms.get(idx).buf();
	const IOObj* ioobj = (*IOM().dirPtr())[nm];
        ioobjids += new MultiID( ioobj ? (const char*)ioobj->key() : "" );
    }
    seisinpfld = new uiLabeledListBox( this, "Select Seismics to merge", true );
    seisinpfld->box()->addItems( ioobjnms );
    seisinpfld->box()->setCurrentItem( 0 );

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
    delete ctio.ioobj; delete &ctio;
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
    const int estnrtrcs = checkRanges();

    /*TODO 
    //TODO get these from cubes
    const int nrsamps = SI().zRange(false).nrSteps() + 1;
    const int bps = 4;
    if ( !estnrtrcs
      || !uiSeisTransfer::checkSpaceLeft(*ctio.ioobj,nrsamps,bps,estnrtrcs) )
	return false;

    const char* txt = "File merger";
    proc = new SeisSingleTraceProc( selobjs, ctio.ioobj, txt, &seliops );
    proc->setTotalNrIfUnknown( estnrtrcs );
    uiExecutor dlg( this, *proc );
    dlg.go();

    if ( remfld->isChecked() )
    {
	for ( int idx=0; idx<inpsz; idx++ )
	{
	    IOObj* ioobj = selobjs[idx];
	    if ( !fullImplRemove(*ioobj) ) 
	    {
		BufferString msg( "Could not remove\n" );
		msg += ioobj->fullUserExpr(YES);
		uiMSG().error( msg );
		continue;
	    }

	    IOM().permRemove( ioobj->key() );
	}
    }

    */
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

    static const char* optdirkey = "Optimized direction";
    static const char* typekey = sKey::Type;
    int order[inpsz];
    int inlstart[inpsz];
    BinIDSampler bs;
    StepInterval<float> zrg;
    StepInterval<float> zrgprev;
    BufferString type = "";
    BufferString optdir = "Vertical";
    for ( int idx=0; idx<inpsz; idx++ )
    {
	IOObj* ioobj = selobjs[idx];
	if ( ioobj->pars().hasKey(typekey) )
	{    
	    if ( !idx ) 
		type = ioobj->pars().find(typekey);
	    else if ( type != ioobj->pars().find(typekey) )
	    {
		uiMSG().error( "Different file types selected" );
		return false;
	    }
	}

	if ( !idx && ioobj->pars().hasKey(optdirkey) )
	    optdir = ioobj->pars().find( optdirkey );

        if ( !SeisTrcTranslator::getRanges( *ioobj, bs, zrg ) )
	{
	    BufferString msg( "Cannot read \"" );
	    msg += ioobj->name(); msg += "\"";
	    uiMSG().error( msg );
	    return false;
	}

	IOPar iopar;
	iopar.set( BinIDSelector::sKeyseltyp, BinIDSelector::keyseltyps()[2] );
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

    if ( type == "" )
	ctio.ioobj->pars().removeWithKey( typekey );
    else
	ctio.ioobj->pars().set( typekey, type );
    ctio.ioobj->pars().set( optdirkey, optdir );
    IOM().commitChanges( *ctio.ioobj );

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


int uiMergeSeis::checkRanges()
{
    StepInterval<int> inlrg, crlrg;
    StepInterval<int> previnlrg, prevcrlrg;
    int estnrtrcs = 0;
    for ( int idx=0; idx<inpsz; idx++ )
    {
	IOPar& iop = *seliops[idx];
	iop.get( BinIDSelector::sKeyfinl, inlrg.start );
	iop.get( BinIDSelector::sKeylinl, inlrg.stop );
	iop.get( BinIDSelector::sKeystepinl, inlrg.step );
	iop.get( BinIDSelector::sKeyfcrl, crlrg.start );
	iop.get( BinIDSelector::sKeylcrl, crlrg.stop );
	iop.get( BinIDSelector::sKeystepcrl, crlrg.step );
	if ( idx )
	{
	    if ( (rev && previnlrg.start <= inlrg.stop)
	     || (!rev && previnlrg.stop >= inlrg.start) )
	    {
	       BufferString msg( "Overlap found in data\n" );
	       msg += "Do you wish to continue?";
	       if ( !uiMSG().askGoOn( msg ) ) return 0;

	       if ( rev )
	       {
		   inlrg.stop = previnlrg.start - inlrg.step;
		   iop.set( BinIDSelector::sKeylinl, inlrg.stop );
	       }
	       else
	       {
		   inlrg.start = previnlrg.stop + inlrg.step;
		   iop.set( BinIDSelector::sKeyfinl, inlrg.start );
	       }
	    }
	}

	estnrtrcs += (inlrg.nrSteps()+1) * (crlrg.nrSteps()+1);
	previnlrg = inlrg;
	prevcrlrg = crlrg;
    }
 
    if ( estnrtrcs < 1 )
    {
	estnrtrcs = 0;
	uiMSG().error( "No traces in selected files" );
    }
    return estnrtrcs;
}
