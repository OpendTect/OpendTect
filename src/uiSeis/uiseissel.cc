/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:		$Id: uiseissel.cc,v 1.49 2008-05-13 14:00:38 cvsbert Exp $
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


static const char* gtSelTxt( const uiSeisSel::Setup& setup, bool forread )
{
    if ( setup.seltxt_ )
	return setup.seltxt_;

    switch ( setup.geom_ )
    {
    case Seis::Vol:
	return forread ? "Input Cube" : "Output Cube";
    case Seis::Line:
	return forread ? "Input Line Set" : "Store in Line Set";
    default:
	return forread ? "Input Data Store" : "Output Data Store";
    }
}


static void adaptCtxt( const IOObjContext& c, const uiSeisSel::Setup& s,
			bool chgtol )
{
    IOObjContext& ctxt = const_cast<IOObjContext&>( c );
    const bool is2d = Seis::is2D(s.geom_);
    // ctxt.trglobexpr = uiSeisSelDlg::standardTranslSel( s.geom_, ctxt.forread );
    ctxt.deftransl = s.geom_ == Seis::Line ? "2D" : "CBVS";
    if ( s.geom_ == Seis::Line && !ctxt.allowcnstrsabsent && chgtol )
	ctxt.allowcnstrsabsent = true;	//change required to get any 2D LineSet
}

static bool kp_allowcnstrsabsent = true; /* hack but should always be OK */


static const CtxtIOObj& getDlgCtio( const CtxtIOObj& c,
				    const uiSeisSel::Setup& s )
{
    kp_allowcnstrsabsent = c.ctxt.allowcnstrsabsent;
    adaptCtxt( c.ctxt, s, true );
    return c;
}



uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const uiSeisSel::Setup& setup )
    : uiIOObjSelDlg(p,getDlgCtio(c,setup),"",false)
    , attrfld_(0)
{
    const bool is2d = Seis::is2D( setup.geom_ );
    const bool isps = Seis::isPS( setup.geom_ );
    allowcnstrsabsent_ = kp_allowcnstrsabsent;
    setTitleText( is2d ? "Select Line Set" : "Select Cube" );

    uiGroup* topgrp = selgrp->getTopGroup();

    if ( setup.selattr_ && is2d && !isps )
    {
	if ( selgrp->getCtxtIOObj().ctxt.forread )
	    attrfld_ = new uiGenInput( selgrp,"Attribute",StringListInpSpec() );
	else
	    attrfld_ = new uiGenInput( selgrp, "Attribute",
		    		       StringInpSpec(LineKey::sKeyDefAttrib) );
	if ( selgrp->getNameField() )
	    attrfld_->attach( alignedBelow, selgrp->getNameField() );
	else
	    attrfld_->attach( ensureBelow, topgrp );
    }

    selgrp->getListField()->selectionChanged.notify(
	    			mCB(this,uiSeisSelDlg,entrySel) );
    entrySel(0);
}


static const char* trglobexprs[] = { "2D", "CBVS", "CBVS`PS Cube" };

const char* uiSeisSelDlg::standardTranslSel( Seis::GeomType geom, bool forread )
{
    return Seis::isPS(geom) ? trglobexprs[1]
	: (Seis::is2D(geom) ? trglobexprs[0] : trglobexprs[2]);
}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    // ioobj should already be filled by base class
    const IOObj* ioobj = ioObj();
    if ( !ioobj || !attrfld_ )
	return;

    uiSeisIOObjInfo oinf( *ioobj, false );
    const bool is2d = oinf.is2D();
    const bool isps = oinf.isPS();
    attrfld_->display( is2d && !isps );
    if ( !selgrp->getCtxtIOObj().ctxt.forread ) return;

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


uiSeisSel::uiSeisSel( uiParent* p, CtxtIOObj& c, const uiSeisSel::Setup& setup )
	: uiIOObjSel(p,c,gtSelTxt(setup,c.ctxt.forread),setup.withclear_)
	, dlgiopar(*new IOPar)
    	, setup_(setup)
{
    adaptCtxt( c.ctxt, setup_, false );
}


uiSeisSel::~uiSeisSel()
{
    delete &dlgiopar;
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
    return !is2D() || isPS() ? uiIOObjSel::existingTyped()
	 : existingUsrName( LineKey(getInput()).lineName() );
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
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, ctio, setup_ );
    dlg->usePar( dlgiopar );
    return dlg;
}
