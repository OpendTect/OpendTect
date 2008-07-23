/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          July 2001
 RCS:		$Id: uiseissel.cc,v 1.57 2008-07-23 09:40:48 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiseissel.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "iodirentry.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "seisselection.h"
#include "seistype.h"
#include "separstr.h"
#include "survinfo.h"


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
    ctxt.trglobexpr = uiSeisSelDlg::standardTranslSel( s.geom_,
	    					       ctxt.forread );
    ctxt.deftransl = s.geom_ == Seis::Line ? "2D" : "CBVS";
    if ( s.geom_ == Seis::Line && !ctxt.allowcnstrsabsent && chgtol )
	ctxt.allowcnstrsabsent = true;	//change required to get any 2D LineSet
}


static const CtxtIOObj& getDlgCtio( const CtxtIOObj& c,
				    const uiSeisSel::Setup& s )
{
    adaptCtxt( c.ctxt, s, true );
    return c;
}



uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const uiSeisSel::Setup& setup )
    : uiIOObjSelDlg(p,getDlgCtio(c,setup),"",false)
    , attrfld_(0)
    , datatype_(0)
{
    const bool is2d = Seis::is2D( setup.geom_ );
    const bool isps = Seis::isPS( setup.geom_ );

    if ( setup.datatype_ )     datatype_ = setup.datatype_;
    allowcnstrsabsent_ = setup.allowcnstrsabsent_;
    include_ = setup.include_;

    setTitleText( isps ? "Select Data Store"
	    	: (is2d ? "Select Line Set" : "Select Cube") );

    uiGroup* topgrp = selgrp->getTopGroup();

    if ( setup.selattr_ && is2d && !isps )
    {
	if ( selgrp->getCtxtIOObj().ctxt.forread )
	{
	    attrfld_ = new uiGenInput( selgrp,"Attribute",StringListInpSpec() );

	    if ( selgrp->getNameField() )
		 attrfld_->attach( alignedBelow, selgrp->getNameField() );
	    else
		attrfld_->attach( ensureBelow, topgrp );
	}
	else
	{
	    attrfld_ = new uiGenInput( selgrp, "Attribute",
		    		       StringInpSpec(sKey::Steering) );
	    attrlistfld_ = new uiListBox( selgrp, "Existing List" );

	    if ( selgrp->getNameField() )
		attrlistfld_->attach( alignedBelow, selgrp->getNameField() );
	    else
		attrlistfld_->attach( ensureBelow, topgrp );

	    attrlistfld_->selectionChanged.notify(
		    		mCB(this,uiSeisSelDlg,attrNmSel) );
	    attrfld_->attach( alignedBelow, attrlistfld_ );
	}
    }

    selgrp->getListField()->selectionChanged.notify(
	    			mCB(this,uiSeisSelDlg,entrySel) );
    if ( !selgrp->getCtxtIOObj().ctxt.forread && Seis::is2D(setup.geom_) )
	selgrp->setConfirmOverwrite( false );
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

    SeisIOObjInfo oinf( *ioobj );
    const bool is2d = oinf.is2D();
    const bool isps = oinf.isPS();
    attrfld_->display( is2d && !isps );

    BufferStringSet nms;
    oinf.getAttribNames( nms, true, 0, getDataType(), 
	    		 allowcnstrsabsent_, include_ );

    if ( selgrp->getCtxtIOObj().ctxt.forread )
	attrfld_->newSpec( StringListInpSpec(nms), 0 );
    else
    {
	const BufferString attrnm( attrfld_->text() );
        attrlistfld_->empty();
	attrlistfld_->addItems( nms ); 
	attrfld_->setText( attrnm );
    }
}


void uiSeisSelDlg::attrNmSel( CallBacker* )
{
    attrfld_->setText( attrlistfld_->getText() );
}


const char* uiSeisSelDlg::getDataType()
{
    if ( !datatype_ )
	return 0;

    static BufferString typekey;
    typekey.setEmpty();
    switch ( Seis::dataTypeOf(datatype_) )
    {
	case Seis::Dip:
	    typekey += sKey::Steering;
	    break;

	// TODO: support other datatypes

	default:
	    return 0;
    }

    return typekey.buf();
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
    if ( attrnm.isEmpty() )
	dlgiopar.removeWithKey( sKey::Attribute );
    else
	dlgiopar.set( sKey::Attribute, nm );
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
    attrnm = iop.find( sKey::Attribute );
}


void uiSeisSel::updateInput()
{
    BufferString ioobjkey;
    if ( ctio.ioobj ) ioobjkey = ctio.ioobj->key();
    setInput( LineKey(ioobjkey,attrnm) );
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
