/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:		$Id: uimergeseis.cc,v 1.36 2007-11-29 14:36:04 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimergeseis.h"

#include "bufstringset.h"
#include "cubesampling.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "seissingtrcproc.h"
#include "seistrc.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "sorting.h"
#include "survinfo.h"
#include "varlenarray.h"

#include "uibutton.h"
#include "uiexecutor.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseistransf.h"

#include <math.h>


uiMergeSeis::uiMergeSeis( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Seismic file merging",
				 "Specify input/output seismics",
				 "103.1.2"))
    , ctio(*mMkCtxtIOObj(SeisTrc))
    , inpsz(0)
    , rev(false)
{
    IOM().to( ctio.ctxt.getSelKey() );
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
    seisoutfld = new uiSeisSel( this, ctio, Seis::SelSetup(false) );
    seisoutfld->attach( alignedBelow, seisinpfld );

    remfld = new uiCheckBox( this, "Remove original files on succes" );
    remfld->attach( alignedBelow, seisoutfld );
    remfld->setChecked( false );
    remfld->display( false ); // TODO: check if merge is really succesfull
}


uiMergeSeis::~uiMergeSeis()
{
    delete ctio.ioobj; delete &ctio;
    deepErase( ioobjids );
    deepErase( selobjs );
    deepErase( seliops );
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

    SeisIOObjInfo::SpaceInfo spi;
    int nrsamps, bps;
    if ( !handleInput(spi.expectednrsamps,spi.maxbytespsamp) )
	return false;
    spi.expectednrtrcs = checkRanges();

    if ( !spi.expectednrtrcs
      || !uiSeisIOObjInfo(*ctio.ioobj).checkSpaceLeft(spi) )
	return false;

    const char* txt = "File merger";
    PtrMan<SeisSingleTraceProc> proc = 
		new SeisSingleTraceProc( selobjs, ctio.ioobj, txt, &seliops );
    proc->setTotalNrIfUnknown( spi.expectednrtrcs );
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

    return true;
}


bool uiMergeSeis::handleInput( int& nrsamps, int& bps )
{
    selobjs.erase(); seliops.erase();
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
    mVariableLengthArr( int, order, inpsz );
    mVariableLengthArr( int, inlstart, inpsz );
    CubeSampling cs;
    StepInterval<float> prevzrg;
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

	uiSeisIOObjInfo oinf( *ioobj );
        if ( !oinf.getRanges(cs) )
	    return false;

	if ( !idx )
	{
	    nrsamps = cs.zrg.nrSteps() + 1;
	    oinf.getBPS( bps );
	}

	IOPar* iopar = new IOPar;
	iopar->set( sKey::BinIDSel, sKey::Range );
	iopar->set( sKey::FirstInl, cs.hrg.start.inl );
        iopar->set( sKey::LastInl, cs.hrg.stop.inl );
        iopar->set( sKey::StepInl, abs(cs.hrg.step.inl) );
	iopar->set( sKey::FirstCrl, cs.hrg.start.crl );
        iopar->set( sKey::LastCrl, cs.hrg.stop.crl );
        iopar->set( sKey::StepCrl, abs(cs.hrg.step.crl) );
	seliops += iopar;
	order[idx] = idx;
	inlstart[idx] = cs.hrg.start.inl;
	if ( cs.hrg.step.inl < 0 ) 
        { 
	    inlstart[idx] *= -1; 
	    rev = true; 
	}

	if ( !idx )
	    prevzrg = cs.zrg;
	else if ( !cs.zrg.isEqual(prevzrg,SI().zStep()/1000 ) )
	{
	    uiMSG().error( "Sorry, not implemented:\n"
		    	   "Merge with different Z-ranges" );
	    return false;
	}
    }

    if ( type.isEmpty() )
	ctio.ioobj->pars().removeWithKey( typekey );
    else
	ctio.ioobj->pars().set( typekey, type );
    ctio.ioobj->pars().set( optdirkey, optdir );
    IOM().commitChanges( *ctio.ioobj );

#ifdef __msvc__
    sort_coupled( inlstart.ptr(), order.ptr(), inpsz );
#else
    sort_coupled( inlstart, order, inpsz );
#endif

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
	iop.get( sKey::FirstInl, inlrg.start );
        iop.get( sKey::LastInl, inlrg.stop );
        iop.get( sKey::StepInl, inlrg.step );
        iop.get( sKey::FirstCrl, crlrg.start );
        iop.get( sKey::LastCrl, crlrg.stop );
        iop.get( sKey::StepCrl, crlrg.step );
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
		   iop.set( sKey::LastInl, inlrg.stop );
	       }
	       else
	       {
		   inlrg.start = previnlrg.stop + inlrg.step;
		   iop.set( sKey::FirstInl, inlrg.start );
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
