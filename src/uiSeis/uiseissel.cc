/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:		$Id: uiseissel.cc,v 1.10 2004-08-23 09:50:12 bert Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "uibinidsubsel.h"
#include "uiseissubsel.h"
#include "uiseisioobjinfo.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "ctxtioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "separstr.h"
#include "seistrctr.h"


uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const SeisSelSetup& setup )
	: uiIOObjSelDlg(p,getCtio(c,setup),"Seismic data selection")
	, subsel(0)
{
    setTitleText( setup.seltxt_ );

    if ( setup.subsel_ )
    {
	topgrp->setHAlignObj( listfld );
	subsel = new uiSeisSubSel( this );
	subsel->attach( alignedBelow, topgrp );
	if ( ctio.iopar )
	    subsel->usePar( *ctio.iopar );
    }

    listfld->box()->selectionChanged.notify( mCB(this,uiSeisSelDlg,entrySel) );
    replaceFinaliseCB( mCB(this,uiSeisSelDlg,fillFlds) );
}


const char* uiSeisSelDlg::standardTranslSel( int pol2d )
{
    static FileMultiString fms;
    fms = "";
    if ( pol2d != SeisSelSetup::Only2D )
	fms += "CBVS";
    if ( pol2d != SeisSelSetup::No2D )
	fms += "2D";
    return fms.buf();
}


const CtxtIOObj& uiSeisSelDlg::getCtio( const CtxtIOObj& c,
					const SeisSelSetup& s )
{
    if ( s.stdtrs_ )
    {
	IOObjContext& ctxt = const_cast<IOObjContext&>( c.ctxt );
	ctxt.trglobexpr = standardTranslSel( s.pol2d_ );
    }
    return c;
}


void uiSeisSelDlg::fillFlds( CallBacker* c )
{
    selChg(c);
    entrySel(c);
}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    // ioobj should already be filled by base class
    if ( !ioobj )
	return;

    if ( subsel )
    {
	subsel->set2D( SeisTrcTranslator::is2D(*ioobj) );
	StepInterval<int> inlrg, crlrg; StepInterval<float> zrg;
	if ( !uiSeisIOObjInfo(*ioobj,ctio.ctxt.forread)
				.getRanges(inlrg,crlrg,zrg) )
	    return;

	subsel->setInput( zrg );
	HorSampling hs; hs.set( inlrg, crlrg );
	subsel->setInput( hs );
    }
}


void uiSeisSelDlg::fillPar( IOPar& iopar ) const
{
    uiIOObjSelDlg::fillPar( iopar );
    if ( subsel ) subsel->fillPar( iopar );
}


void uiSeisSelDlg::usePar( const IOPar& iopar )
{
    uiIOObjSelDlg::usePar( iopar );
    if ( subsel ) subsel->usePar( iopar );
}


uiSeisSel::uiSeisSel( uiParent* p, CtxtIOObj& c, const char* txt,
		      const SeisSelSetup& s, bool wclr )
	: uiIOObjSel( p, c,
		txt ? txt : (c.ctxt.forread?"Input seismics":"Output seismics"),
		wclr, s.seltxt_ )
	, iopar(*new IOPar)
	, setup(s)
{
}


uiSeisSel::~uiSeisSel()
{
    delete &iopar;
}


void uiSeisSel::newSelection( uiIOObjRetDlg* dlg )
{
    ((uiSeisSelDlg*)dlg)->fillPar( iopar );
}


bool uiSeisSel::is2D() const
{
    return ctio.ioobj && SeisTrcTranslator::is2D( *ctio.ioobj );
}


bool uiSeisSel::fillPar( IOPar& iop ) const
{
    iop.merge( iopar );
    return uiIOObjSel::fillPar( iop );
}


void uiSeisSel::usePar( const IOPar& iop )
{
    uiIOObjSel::usePar( iop );
    iopar.merge( iop );
}


void uiSeisSel::set2DPol( SeisSelSetup::Pol2D pol )
{
    setup.pol2d_ = pol;
    if ( ctio.ioobj )
    {
	const bool curis2d = SeisTrcTranslator::is2D( *ctio.ioobj );
	if ( (curis2d && pol == SeisSelSetup::No2D)
	  || (!curis2d && pol == SeisSelSetup::Only2D) )
	{
	    ctio.setObj( 0 );
	    updateInput();
	}
    }
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, ctio, setup );
    dlg->usePar( iopar );
    return dlg;
}
