/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:		$Id: uiseissel.cc,v 1.20 2004-09-29 08:01:59 bert Exp $
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


static void mkKvals( const IOObjContext& ctxt, BufferString& keyvals )
{
    keyvals = ctxt.ioparkeyval[0];
    keyvals += "`";
    keyvals += ctxt.ioparkeyval[1];
}


static void resetKeyVals( const IOObjContext& ct, const BufferString& kyvals )
{
    IOObjContext& ctxt = const_cast<IOObjContext&>( ct );
    ctxt.ioparkeyval[0] = kyvals;
    char* ptr = strchr( ctxt.ioparkeyval[0].buf(), '`' );
    if ( ptr ) *ptr++ = '\0';
    ctxt.ioparkeyval[1] = ptr;
}


static void adaptCtxt( IOObjContext& ctxt, SeisSelSetup::Pol2D pol,
			const BufferString& orgkeyvals )
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

    if ( pol == SeisSelSetup::No2D )
	resetKeyVals( ctxt, orgkeyvals );
    else
	ctxt.ioparkeyval[0] = ctxt.ioparkeyval[1] = "";
}


uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const SeisSelSetup& setup )
	: uiIOObjSelDlg(p,getCtio(c,setup),"")
	, subsel(0)
{
    mkKvals( ctio.ctxt, orgkeyvals );
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


uiSeisSelDlg::~uiSeisSelDlg()
{
    resetKeyVals( ctio.ctxt, orgkeyvals );
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
	BufferString kvs; mkKvals( c.ctxt, kvs );
	adaptCtxt( ctxt, s.pol2d_, kvs );
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


static const char* gtSelTxt( const char** sts, SeisSelSetup::Pol2D p2d,
			     bool forread )
{
    // Support:
    // 1) One single text: { "Text", 0 }
    // 2) Same for read and write: { "Text3D", Text3D2D", Text2D", 0 }
    // 3) { "Text3DRead", Text3D2DRead", Text2DRead",
    //      "Text3DWrite", Text3D2DWrite", Text2DWrite", 0 }

    static const char* stdseltxts[] = {
	    "Input Cube", "Input Seismics", "Input Line Set",
	    "Output Cube", "Output Seismics", "Store in Line Set", 0
    };

    if ( !sts ) sts = stdseltxts;
    if ( !sts[1] ) return sts[0];
    if ( !sts[3] ) return sts[(int)p2d];
    return sts[ 3 * (forread ? 0 : 1) + (int)p2d ];
}


uiSeisSel::uiSeisSel( uiParent* p, CtxtIOObj& c, const SeisSelSetup& s,
		      bool wclr, const char** sts )
	: uiIOObjSel(p,c,gtSelTxt(sts,SeisSelSetup::Both2DAnd3D,c.ctxt.forread),
		     wclr )
	, iopar(*new IOPar)
	, setup(s)
    	, seltxts(sts)
{
    mkKvals( ctio.ctxt, orgkeyvals );
    if ( !c.ctxt.forread )
	inp_->label()->setPrefWidthInChar( 15 );

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
    BufferString newdisptxt = gtSelTxt( seltxts, pol, ctio.ctxt.forread );
    if ( newdisptxt != disptxt )
	setLabelText( newdisptxt );

    adaptCtxt( ctio.ctxt, pol, orgkeyvals );
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, ctio, setup );
    dlg->usePar( iopar );
    return dlg;
}
