/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:		$Id: uiseissel.cc,v 1.1 2003-10-01 12:51:43 bert Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "uibinidsubsel.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "ctxtioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "binidselimpl.h"
#include "separstr.h"
#include "seistrctr.h"


uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c, bool withz,
			    const char* s )
	: uiIOObjSelDlg(p,c,s)
	, subsel(0)
{
    setTitleText( "Specify seismic data input" );

    if ( ctio.ctxt.forread )
    {
	topgrp->setHAlignObj( listfld );
	subsel = new uiBinIDSubSel( this, uiBinIDSubSel::Setup()
					  .withtable(true).withz(withz) );
	subsel->attach( alignedBelow, topgrp );
	if ( c.iopar )
	    subsel->usePar( *c.iopar );
    }

    listfld->box()->selectionChanged.notify( mCB(this,uiSeisSelDlg,selChg) );
}


void uiSeisSelDlg::fillPar( IOPar& iopar ) const
{
    uiIOObjSelDlg::fillPar( iopar );
    if ( subsel )
	subsel->fillPar( iopar );
}


void uiSeisSelDlg::usePar( const IOPar& iopar )
{
    uiIOObjSelDlg::usePar( iopar );
    if ( subsel )
	subsel->usePar( iopar );
}


void uiSeisSelDlg::selChg( CallBacker* )
{
    if ( !ioobj )
	return;
    BinIDSampler bs;
    StepInterval<float> zrg;
    if ( subsel && SeisTrcTranslator::getRanges( *ioobj, bs, zrg ) )
    {
	subsel->setInput( bs );
	subsel->setInput( zrg );
    }
}


uiSeisSel::uiSeisSel( uiParent* p, CtxtIOObj& c, const char* txt, bool wz,
		      bool wclr, const char* st )
	: uiIOObjSel( p, c,
		txt ? txt : (c.ctxt.forread?"Input seismics":"Output seismics"),
		wclr, st )
	, withz(wz)
{
}


void uiSeisSel::newSelection( uiIOObjRetDlg* dlg )
{
    if ( !ctio.iopar ) ctio.iopar = new IOPar;
    ((uiSeisSelDlg*)dlg)->fillPar( *ctio.iopar );
}


bool uiSeisSel::fillPar( IOPar& iop ) const
{
    if ( ctio.iopar ) iop.merge( *ctio.iopar );
    return uiIOObjSel::fillPar( iop );
}


void uiSeisSel::usePar( const IOPar& iop )
{
    uiIOObjSel::usePar( iop );
    if ( ctio.iopar ) ctio.iopar->merge( iop );
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    return new uiSeisSelDlg( this, ctio, withz, seltxt );
}
