/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:		$Id: uiseissel.cc,v 1.16 2004-09-15 20:24:46 bert Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "uibinidsubsel.h"
#include "uiseissubsel.h"
#include "uiseisioobjinfo.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "ctxtioobj.h"
#include "iopar.h"
#include "survinfo.h"
#include "cubesampling.h"
#include "separstr.h"
#include "seistrctr.h"


static void adaptCtxt( const IOObjContext& ctxt, SeisSelSetup::Pol2D pol )
{
    BufferString& deftr = const_cast<IOObjContext*>(&ctxt)->deftransl;
    if ( pol == SeisSelSetup::Only2D )
	deftr = "2D";
    else if ( deftr == "2D" )
    {
	BufferString dt( ctxt.trglobexpr );
	char* ptr = strchr( deftr.buf(), '`' );
	if ( ptr ) *ptr = '\0';
	if ( dt == "2D" )
	    deftr = "CBVS";
	else
	    deftr = dt;
    }
}


uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const SeisSelSetup& setup )
	: uiIOObjSelDlg(p,getCtio(c,setup),"")
	, subsel(0)
{
    const char* ttxt = setup.pol2d_ == SeisSelSetup::No2D ? "Select Cube"
							  : "Select Line Set";
    if ( setup.pol2d_ == SeisSelSetup::Both2DAnd3D )
	ttxt = ctio.ctxt.forread ? "Select Input seismics"
	    			 : "Select Output Seismics";
    setTitleText( ttxt );

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

static const char* trglobexprs[] = { "2D", "CBVS`2D", "CBVS" };

const char* uiSeisSelDlg::standardTranslSel( int pol2d )
{
    int nr = pol2d == SeisSelSetup::Only2D ? 0
		    : (pol2d == SeisSelSetup::No2D ? 2 : 1);
    return trglobexprs[nr];
}


const CtxtIOObj& uiSeisSelDlg::getCtio( const CtxtIOObj& c,
					const SeisSelSetup& s )
{
    if ( s.stdtrs_ )
    {
	IOObjContext& ctxt = const_cast<IOObjContext&>( c.ctxt );
	ctxt.trglobexpr = standardTranslSel( s.pol2d_ );
	adaptCtxt( ctxt, s.pol2d_ );
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
	uiSeisIOObjInfo oinf(*ioobj,false);
	subsel->set2D( oinf.is2D() );
	CubeSampling cs;
	if ( oinf.getRanges(cs) )
	    subsel->setInput( cs );
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


uiSeisSel::uiSeisSel( uiParent* p, CtxtIOObj& c, const SeisSelSetup& s,
		      bool wclr )
	: uiIOObjSel( p, c, (c.ctxt.forread?"Input Seismics":"Store as Line Set"),
		      wclr )
	, iopar(*new IOPar)
	, setup(s)
{
    set2DPol( setup.pol2d_ );
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
    BufferString disptxt = labelText();
    BufferString newdisptxt = ctio.ctxt.forread ? "Input Seismics" :
	(pol == SeisSelSetup::Only2D ? "Store in Line Set"
      : (pol == SeisSelSetup::No2D ? "Store as Cube" : "Output Seismics") );
    if ( newdisptxt != disptxt )
	setLabelText( newdisptxt );

    adaptCtxt( ctio.ctxt, pol );
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, ctio, setup );
    dlg->usePar( iopar );
    return dlg;
}
