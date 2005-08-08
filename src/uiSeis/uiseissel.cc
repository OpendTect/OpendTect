/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:		$Id: uiseissel.cc,v 1.32 2005-08-08 15:13:51 cvsnanne Exp $
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
#include "ioobj.h"
#include "iodirentry.h"
#include "survinfo.h"
#include "seistrcsel.h"
#include "cubesampling.h"
#include "separstr.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "linekey.h"
#include "keystrs.h"


uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const SeisSelSetup& setup )
	: uiIOObjSelDlg(p,getCtio(c,setup),"",false)
	, p2d(setup.pol2d_)
	, subsel(0)
	, attrfld(0)
{
    const char* ttxt = p2d == No2D ? "Select Cube" : "Select Line Set";
    if ( p2d == Both2DAnd3D )
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

    if ( setup.selattr_ && p2d != No2D )
    {
	if ( ctio.ctxt.forread )
	    attrfld = new uiGenInput( this, "Attribute", StringListInpSpec() );
	else
	    attrfld = new uiGenInput( this, "Attribute (if any)" );
	if ( subsel )
	    attrfld->attach( alignedBelow, subsel );
	else if ( nmfld )
	    attrfld->attach( alignedBelow, nmfld );
	else
	    attrfld->attach( ensureBelow, topgrp );
    }

    listfld->box()->selectionChanged.notify( mCB(this,uiSeisSelDlg,entrySel) );
    selChg(0); entrySel(0);
}


static const char* trglobexprs[] = { "2D", "CBVS`2D", "CBVS" };

const char* uiSeisSelDlg::standardTranslSel( Pol2D pol2d )
{
    int nr = pol2d == Only2D ? 0 : (pol2d == No2D ? 2 : 1);
    return trglobexprs[nr];
}


static void adaptCtxt( const IOObjContext& c, Pol2D p2d )
{
    IOObjContext& ctxt = const_cast<IOObjContext&>( c );
    ctxt.trglobexpr = uiSeisSelDlg::standardTranslSel( p2d );
    ctxt.deftransl = p2d == Only2D ? trglobexprs[0] : trglobexprs[2];
    if ( p2d != No2D )
	ctxt.parconstraints.clear(); // Selection is done differently
}


const CtxtIOObj& uiSeisSelDlg::getCtio( const CtxtIOObj& c,
					const SeisSelSetup& s )
{
    adaptCtxt( c.ctxt, s.pol2d_ );
    return c;
}


uiSeisSelDlg::~uiSeisSelDlg()
{
}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    // ioobj should already be filled by base class
    if ( !ioobj )
	return;

    uiSeisIOObjInfo oinf(*ioobj,false);
    if ( subsel )
    {
	subsel->set2D( oinf.is2D() );
	subsel->setInput( *ioobj );
    }

    if ( !attrfld ) return;

    const bool is2d = oinf.is2D();
    attrfld->display( is2d );
    if ( !is2d || !ctio.ctxt.forread ) return;

    BufferStringSet nms;
    oinf.getAttribNames( nms );
    attrfld->newSpec( StringListInpSpec(nms), 0 );
}


void uiSeisSelDlg::fillPar( IOPar& iopar ) const
{
    uiIOObjSelDlg::fillPar( iopar );
    if ( subsel ) subsel->fillPar( iopar );
    if ( attrfld ) iopar.set( sKey::Attribute, attrfld->text() );
}


void uiSeisSelDlg::usePar( const IOPar& iopar )
{
    uiIOObjSelDlg::usePar( iopar );
    if ( subsel ) subsel->usePar( iopar );
    if ( attrfld )
    {
	entrySel(0);
	const char* selattrnm = iopar.find( sKey::Attribute );
	if ( selattrnm ) attrfld->setText( selattrnm );
    }
}


static const char* gtSelTxt( const char** sts, Pol2D p2d, bool forread )
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

    const int offs = (int)p2d - (int)No2D;
    return sts[3] ? sts[ 3 * (forread ? 0 : 1) + offs ] : sts[offs];
}


uiSeisSel::uiSeisSel( uiParent* p, CtxtIOObj& c, const SeisSelSetup& s,
		      bool wclr, const char** sts )
	: uiIOObjSel(p,c,gtSelTxt(sts,Both2DAnd3D,c.ctxt.forread),wclr)
	, setup(*new SeisSelSetup(s))
    	, seltxts(sts)
	, dlgiopar(*new IOPar)
	, orgparconstraints(*new IOPar(c.ctxt.parconstraints))
{
    if ( !c.ctxt.forread )
	inp_->label()->setPrefWidthInChar( 15 );

    set2DPol( setup.pol2d_ );
}


uiSeisSel::~uiSeisSel()
{
    delete &setup;
    delete &dlgiopar;
    delete &orgparconstraints;
}


void uiSeisSel::newSelection( uiIOObjRetDlg* dlg )
{
    ((uiSeisSelDlg*)dlg)->fillPar( dlgiopar );
    setAttrNm( dlgiopar.find( sKey::Attribute ) );
}


void uiSeisSel::setAttrNm( const char* nm )
{
    attrnm = nm;
    updateInput();
}


const char* uiSeisSel::userNameFromKey( const char* txt ) const
{
    if ( !txt || !*txt ) return "";

    LineKey lk( txt );
    curusrnm = uiIOObjSel::userNameFromKey( lk.lineName() );
    curusrnm = LineKey( curusrnm, lk.attrName() );
    return curusrnm.buf();
}


bool uiSeisSel::existingTyped() const
{
    return !is2D() ? uiIOObjSel::existingTyped()
	 : existingUsrName( LineKey(getInput()).lineName() );
}


bool uiSeisSel::is2D() const
{
    return ctio.ioobj && SeisTrcTranslator::is2D( *ctio.ioobj );
}


bool uiSeisSel::fillPar( IOPar& iop ) const
{
    iop.merge( dlgiopar );
    return uiIOObjSel::fillPar( iop );
}


void uiSeisSel::usePar( const IOPar& iop )
{
    uiIOObjSel::usePar( iop );
    dlgiopar.merge( iop );
}


void uiSeisSel::updateInput()
{
    if ( !ctio.ioobj ) return;
    setInput( LineKey(ctio.ioobj->key(),attrnm) );
}


void uiSeisSel::processInput()
{
    obtainIOObj();
    attrnm = LineKey( getInput() ).attrName();
    if ( ctio.ioobj || ctio.ctxt.forread )
	updateInput();
}


void uiSeisSel::set2DPol( Pol2D pol )
{
    setup.pol2d_ = pol;
    if ( ctio.ioobj )
    {
	const bool curis2d = SeisTrcTranslator::is2D( *ctio.ioobj );
	if ( (curis2d && pol == No2D) || (!curis2d && pol == Only2D) )
	{
	    ctio.setObj( 0 );
	    updateInput();
	}
    }
    BufferString disptxt = labelText();
    BufferString newdisptxt = gtSelTxt( seltxts, pol, ctio.ctxt.forread );
    if ( newdisptxt != disptxt )
	setLabelText( newdisptxt );

    ctio.ctxt.parconstraints = orgparconstraints;
    adaptCtxt( ctio.ctxt, pol );
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, ctio, setup );
    dlg->usePar( dlgiopar );
    return dlg;
}
