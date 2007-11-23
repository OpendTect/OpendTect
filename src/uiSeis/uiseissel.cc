/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:		$Id: uiseissel.cc,v 1.41 2007-11-23 11:59:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"
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
#include "cubesampling.h"
#include "separstr.h"
#include "seisselection.h"
#include "linekey.h"
#include "keystrs.h"



static void adaptCtxt( const IOObjContext& c, bool is2d, bool chgtol )
{
    IOObjContext& ctxt = const_cast<IOObjContext&>( c );
    ctxt.trglobexpr = uiSeisSelDlg::standardTranslSel( is2d );
    ctxt.deftransl = uiSeisSelDlg::standardTranslSel( is2d );
    if ( is2d && !ctxt.allowcnstrsabsent && chgtol )
	ctxt.allowcnstrsabsent = true;	//change required to get any 2D LineSet
}

static bool kp_allowcnstrsabsent = true; /* hack but should always be OK */


static const CtxtIOObj& getDlgCtio( const CtxtIOObj& c,
				    const Seis::SelSetup& s )
{
    kp_allowcnstrsabsent = c.ctxt.allowcnstrsabsent;
    adaptCtxt( c.ctxt, s.is2d_, true );
    return c;
}



uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const Seis::SelSetup& selsetup )
    : uiIOObjSelDlg(p,getDlgCtio(c,selsetup),"",false)
    , is2d_(selsetup.is2d_)
    , attrfld_(0)
{
    allowcnstrsabsent_ = kp_allowcnstrsabsent;
    const char* ttxt = is2d_ ? "Select Line Set" : "Select Cube";
    setTitleText( ttxt );

    uiGroup* topgrp = selgrp->getTopGroup();

    if ( selsetup.selattr_ && is2d_ )
    {
	if ( selgrp->getCtxtIOObj().ctxt.forread )
	    attrfld_ = new uiGenInput( selgrp,"Attribute",StringListInpSpec() );
	else
	    attrfld_ = new uiGenInput( selgrp, "Attribute (if any)" );
	if ( selgrp->getNameField() )
	    attrfld_->attach( alignedBelow, selgrp->getNameField() );
	else
	    attrfld_->attach( ensureBelow, topgrp );
    }

    selgrp->getListField()->selectionChanged.notify(
	    			mCB(this,uiSeisSelDlg,entrySel) );
    entrySel(0);
}


static const char* trglobexprs[] = { "2D", "CBVS" };

const char* uiSeisSelDlg::standardTranslSel( bool is2d )
{
    return trglobexprs[is2d ? 0 : 1];
}

uiSeisSelDlg::~uiSeisSelDlg()
{
}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    // ioobj should already be filled by base class
    const IOObj* ioobj = ioObj();
    if ( !ioobj || !attrfld_ )
	return;

    uiSeisIOObjInfo oinf( *ioobj, false );
    const bool is2d = oinf.is2D();
    attrfld_->display( is2d );
    if ( !is2d || !selgrp->getCtxtIOObj().ctxt.forread ) return;

    BufferStringSet nms;
    oinf.getAttribNames( nms );
    filter2DStoredNames( nms );
    attrfld_->newSpec( StringListInpSpec(nms), 0 );
}


void uiSeisSelDlg::filter2DStoredNames( BufferStringSet& nms ) const
{
    BufferString tcstraint;
    if ( selgrp->getCtxtIOObj().ctxt.parconstraints.get( sKey::Type, tcstraint)
         && !strcmp( tcstraint, sKey::Steering ) )
    {
	bool inccstraints = selgrp->getCtxtIOObj().ctxt.includeconstraints;
	for ( int idx=nms.size()-1; idx>=0; idx-- )
	{
	    int cmp = strncmp( sKey::Steering, nms[idx]->buf(), 8 );
	    if ( inccstraints && cmp && !allowcnstrsabsent_ )
		nms.remove( idx );
	    else if ( !cmp && !inccstraints )
		nms.remove( idx );
	}
    }
}


void uiSeisSelDlg::fillPar( IOPar& iopar ) const
{
    uiIOObjSelDlg::fillPar( iopar );
    if ( attrfld_ ) iopar.set( sKey::Attribute, attrfld_->text() );
}


void uiSeisSelDlg::usePar( const IOPar& iopar )
{
    uiIOObjSelDlg::usePar( iopar );
    if ( attrfld_ )
    {
	entrySel(0);
	const char* selattrnm = iopar.find( sKey::Attribute );
	if ( selattrnm ) attrfld_->setText( selattrnm );
    }
}


static const char* gtSelTxt( const char** sts, bool is2d, bool forread )
{
    // Support:
    // 1) One single text: { "Text", 0 }
    // 2) Same for read and write: { "Text3D", Text2D", 0 }
    // 3) { "Text3DRead", Text2DRead", "Text3DWrite", Text2DWrite", 0 }

    static const char* stdseltxts[] = {
	"Input Cube", "Input Line Set", "Output Cube", "Store in Line Set", 0 };

    if ( !sts ) sts = stdseltxts;

    if ( !sts[1] ) return sts[0];

    const int offs = is2d ? 1 : 0;
    return sts[2] ? sts[ 2 * (forread ? 0 : 1) + offs ] : sts[offs];
}


uiSeisSel::uiSeisSel( uiParent* p, CtxtIOObj& c, const Seis::SelSetup& s,
		      bool wclr, const char** sts )
	: uiIOObjSel(p,c,gtSelTxt(sts,s.is2d_,c.ctxt.forread),wclr)
	, setup(*new Seis::SelSetup(s))
    	, seltxts(sts)
	, dlgiopar(*new IOPar)
	, orgparconstraints(*new IOPar(c.ctxt.parconstraints))
{
    if ( !c.ctxt.forread )
	inp_->label()->setPrefWidthInChar( 15 );

    adaptCtxt( c.ctxt, setup.is2d_ , false );
    setLabelText( gtSelTxt( seltxts, is2D(), ctio.ctxt.forread ) );
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
    return setup.is2d_;
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


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, ctio, setup );
    dlg->usePar( dlgiopar );
    return dlg;
}
