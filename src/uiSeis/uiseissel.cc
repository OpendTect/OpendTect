/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uiseissel.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uibutton.h"

#include "zdomain.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "iodirentry.h"
#include "iostrm.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "seistype.h"
#include "separstr.h"
#include "survinfo.h"
#include "seiscbvs.h"
#include "seispsioprov.h"


static const char* gtSelTxt( const uiSeisSel::Setup& setup, bool forread )
{
    if ( !setup.seltxt_.isEmpty() )
	return setup.seltxt_.buf();

    switch ( setup.geom_ )
    {
    case Seis::Vol:
	return forread ? "Input Cube" : "Output Cube";
    case Seis::Line:
	return forread ? (setup.selattr_ ? "Input Line Set|Attribute"
					 : "Input Line Set")
	    	       : (setup.selattr_ ? "Output Line Set|Attribute"
			       		 : "Output Line Set");
    default:
	return forread ? "Input Data Store" : "Output Data Store";
    }
}


static void adaptCtxt( const IOObjContext& ct, const uiSeisSel::Setup& su,
			bool chgtol )
{
    IOObjContext& ctxt = const_cast<IOObjContext&>( ct );

    if ( su.geom_ == Seis::Line )
	ctxt.toselect.allowtransls_ = "2D";
    else
    {
	if ( su.steerpol_ == uiSeisSel::Setup::NoSteering )
	    ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );
	else if ( su.steerpol_ == uiSeisSel::Setup::OnlySteering )
	    ctxt.toselect.require_.set( sKey::Type(), sKey::Steering() );
    }

    if ( ctxt.deftransl.isEmpty() )
    {
	ctxt.deftransl = su.geom_ == Seis::Line ? "2D"
				: CBVSSeisTrcTranslator::translKey();
	ctxt.toselect.allowtransls_ = ctxt.deftransl;
    }
    else if ( !ctxt.forread )
	ctxt.toselect.allowtransls_ = ctxt.deftransl;
    else if ( !ctxt.toselect.allowtransls_.isEmpty() )
    {
	FileMultiString fms( ctxt.toselect.allowtransls_ );
	if ( fms.indexOf(ctxt.deftransl.buf()) < 0 )
	{
	    fms += ctxt.deftransl;
	    ctxt.toselect.allowtransls_ = fms;
	}
    }
}


static const CtxtIOObj& getDlgCtio( const CtxtIOObj& c,
				    const uiSeisSel::Setup& s )
{
    adaptCtxt( c.ctxt, s, true );
    return c;
}



uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const uiSeisSel::Setup& sssu )
    : uiIOObjSelDlg(p,getDlgCtio(c,sssu),"",false)
    , attrfld_(0)
    , compfld_(0)
    , attrlistfld_(0)
    , steerpol_(sssu.steerpol_)
    , zdomainkey_(sssu.zdomkey_)
{
    const bool is2d = Seis::is2D( sssu.geom_ );
    const bool isps = Seis::isPS( sssu.geom_ );

    if ( is2d && !sssu.allowlinesetsel_ )
    {
	selgrp_->getTopGroup()->display( false, true );
	selgrp_->getNameField()->display( false, true );

	if ( c.ioobj )
	{
	    TypeSet<MultiID> selmids;
	    selmids += c.ioobj->key();
	    selgrp_->setSelected( selmids );
	}
    }

    BufferString titletxt( "Select " );
    if ( sssu.seltxt_ )
	titletxt += sssu.seltxt_;
    else
	titletxt += isps ? "Data Store" : (is2d ? "Line Set" : "Cube");
    setTitleText( titletxt );

    uiGroup* topgrp = selgrp_->getTopGroup();

    if ( sssu.selattr_ && is2d && !isps )
    {
	if ( selgrp_->getCtxtIOObj().ctxt.forread )
	{
	    attrfld_ = new uiGenInput(selgrp_,"Attribute",StringListInpSpec());

	    if ( selgrp_->getNameField() )
		attrfld_->attach( alignedBelow, selgrp_->getNameField() );
	    else
		attrfld_->attach( ensureBelow, topgrp );
	}
	else
	{
	    attrfld_ = new uiGenInput( selgrp_, "Attribute" );
	    attrlistfld_ = new uiListBox( selgrp_, "Existing List" );

	    if ( selgrp_->getNameField() )
		attrlistfld_->attach( alignedBelow, selgrp_->getNameField() );
	    else
		attrlistfld_->attach( ensureBelow, topgrp );

	    const CallBack cb( mCB(this,uiSeisSelDlg,attrNmSel) );
	    attrlistfld_->selectionChanged.notify( cb );
	    attrlistfld_->doubleClicked.notify( cb );
	    attrfld_->attach( alignedBelow, attrlistfld_ );
	}
    }

    selgrp_->getListField()->selectionChanged.notify(
	    			mCB(this,uiSeisSelDlg,entrySel) );
    if ( !selgrp_->getCtxtIOObj().ctxt.forread && Seis::is2D(sssu.geom_) )
	selgrp_->setConfirmOverwrite( false );
    entrySel(0);
    if ( attrlistfld_ && selgrp_->getCtxtIOObj().ctxt.forread )
	attrNmSel(0);

    if ( selgrp_->getCtxtIOObj().ctxt.forread && sssu.selectcomp_ )
    {
	compfld_ = new uiLabeledComboBox( this, "Component", "Compfld" );
	if ( attrfld_ )
	    compfld_->attach( rightTo, attrfld_ );
	else
	    compfld_->attach( alignedBelow, selgrp_ );

	entrySel(0);
    }
}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    // ioobj should already be filled by base class
    const IOObj* ioobj = ioObj();
    if ( !ioobj || ( !attrfld_ && !compfld_ ) )
	return;

    IOObjContext ctxt = selgrp_->getCtxtIOObj().ctxt;

    BufferStringSet nms;
    if ( attrfld_ )
    {
	SeisIOObjInfo oinf( *ioobj );
	const bool is2d = oinf.is2D();
	const bool isps = oinf.isPS();
	attrfld_->display( is2d && !isps );
	SeisIOObjInfo::Opts2D o2d;
	o2d.steerpol_ = (int)steerpol_;
	o2d.zdomky_ = zdomainkey_;
	oinf.getAttribNames( nms, o2d );
    }

    if ( ctxt.forread )
    {
	if ( attrfld_ )
	{
	    const int defidx = nms.indexOf( LineKey::sKeyDefAttrib() );
	    attrfld_->newSpec( StringListInpSpec(nms), 0 );
	    if ( defidx >= 0 )
		attrfld_->setValue( defidx );
	}

	if ( compfld_ )
	{
	    compfld_->box()->setCurrentItem(0);
	    SeisTrcReader rdr( ioobj );
	    if ( !rdr.prepareWork(Seis::PreScan) ) return;
	    SeisTrcTranslator* transl = rdr.seisTranslator();
	    if ( !transl ) return;
	    BufferStringSet compnms;
	    transl->getComponentNames( compnms );
	    compfld_->box()->setEmpty();
	    compfld_->box()->addItems( compnms );
	    compfld_->display( transl->componentInfo().size()>1 );
	}
    }
    else
    {
	if ( !attrfld_ ) return;

	const BufferString attrnm( attrfld_->text() );
        attrlistfld_->setEmpty();
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
    if ( steerpol_ )
	return steerpol_ == uiSeisSel::Setup::NoSteering ? 0 : sKey::Steering();
    const IOObj* ioobj = ioObj();
    if ( !ioobj ) return 0;
    const char* res = ioobj->pars().find( sKey::Type() );
    return res;
}


void uiSeisSelDlg::fillPar( IOPar& iopar ) const
{
    uiIOObjSelDlg::fillPar( iopar );
    if ( attrfld_ ) iopar.set( sKey::Attribute(), attrfld_->text() );
    if ( compfld_ )
    {
	BufferStringSet compnms;
	getComponentNames( compnms );
	const int compnr = compnms.indexOf( compfld_->box()->text() );
	if ( compnr>=0 )
	    iopar.set( sKey::Component(), compnr );
    }
}


void uiSeisSelDlg::usePar( const IOPar& iopar )
{
    uiIOObjSelDlg::usePar( iopar );

    if ( attrfld_ || compfld_ )
	entrySel(0);

    if ( attrfld_ )
    {
	const char* selattrnm = iopar.find( sKey::Attribute() );
	if ( selattrnm ) attrfld_->setText( selattrnm );
    }
    if ( compfld_ )
    {
	int selcompnr = mUdf(int);
	if ( iopar.get( sKey::Component(), selcompnr ) && !mIsUdf( selcompnr) )
	{
	    BufferStringSet compnms;
	    getComponentNames( compnms );
	    if ( selcompnr >= compnms.size() ) return;
	    compfld_->box()->setText( compnms.get(selcompnr).buf() );
	}
    }
}


void uiSeisSelDlg::getComponentNames( BufferStringSet& compnms ) const
{
    compnms.erase();
    const IOObj* ioobj = ioObj();
    if ( !ioobj ) return;
    SeisTrcReader rdr( ioobj );                                         
    if ( !rdr.prepareWork(Seis::PreScan) ) return;
    SeisTrcTranslator* transl = rdr.seisTranslator();
    if ( !transl ) return;                     
    transl->getComponentNames( compnms );
}


static CtxtIOObj& getCtxtIOObj( CtxtIOObj& c, const uiSeisSel::Setup& s )
{
    adaptCtxt( c.ctxt, s, true );
    return c;
}


static const IOObjContext& getIOObjCtxt( const IOObjContext& c,
					 const uiSeisSel::Setup& s )
{
    adaptCtxt( c, s, true );
    return c;
}


uiSeisSel::uiSeisSel( uiParent* p, const IOObjContext& ctxt,
		      const uiSeisSel::Setup& su )
	: uiIOObjSel(p,getIOObjCtxt(ctxt,su),mkSetup(su,ctxt.forread))
    	, seissetup_(mkSetup(su,ctxt.forread))
    	, othdombox_(0)
        , compnr_(0)
{
    workctio_.ctxt = inctio_.ctxt;
    if ( !ctxt.forread && Seis::is2D(seissetup_.geom_) )
	seissetup_.confirmoverwr_ = setup_.confirmoverwr_ = false;

    mkOthDomBox();
    updateInput();
}


uiSeisSel::uiSeisSel( uiParent* p, CtxtIOObj& c, const uiSeisSel::Setup& su )
	: uiIOObjSel(p,getCtxtIOObj(c,su),mkSetup(su,c.ctxt.forread))
    	, seissetup_(mkSetup(su,c.ctxt.forread))
    	, othdombox_(0)
        , compnr_(0)
{
    workctio_.ctxt = inctio_.ctxt;
    if ( !c.ctxt.forread && Seis::is2D(seissetup_.geom_) )
	seissetup_.confirmoverwr_ = setup_.confirmoverwr_ = false;

    mkOthDomBox();
    updateInput();
}


void uiSeisSel::mkOthDomBox()
{
    if ( !inctio_.ctxt.forread && seissetup_.enabotherdomain_ )
    {
	othdombox_ = new uiCheckBox( this, SI().zIsTime() ? "Depth" : "Time" );
	othdombox_->attach( rightOf, selbut_ );
    }
}


uiSeisSel::~uiSeisSel()
{
}


uiSeisSel::Setup uiSeisSel::mkSetup( const uiSeisSel::Setup& su, bool forread )
{
    uiSeisSel::Setup ret( su );
    ret.seltxt_ = gtSelTxt( su, forread );
    ret.filldef( su.allowsetdefault_ );
    return ret;
}


CtxtIOObj* uiSeisSel::mkCtxtIOObj( Seis::GeomType gt, bool forread )
{
    const bool is2d = Seis::is2D( gt );

    CtxtIOObj* ret;
    if ( Seis::isPS(gt) )
    {
	ret = is2d ? mMkCtxtIOObj(SeisPS2D) : mMkCtxtIOObj(SeisPS3D);
	if ( forread )
	    ret->fillDefault();
    }
    else
    {
	ret = mMkCtxtIOObj(SeisTrc);
	if ( forread )
	    ret->fillDefaultWithKey( is2d ? sKey::DefLineSet() : sKey::DefCube() );
    }

    ret->ctxt.forread = forread;
    return ret;
}


IOObjContext uiSeisSel::ioContext( Seis::GeomType geom, bool forread )
{
    IOObjContext ctxt( Seis::isPS(geom)
	    ? (Seis::is2D(geom)
	        ? mIOObjContext(SeisPS2D)
		: mIOObjContext(SeisPS3D) )
	    : mIOObjContext(SeisTrc) );
    fillContext( geom, forread, ctxt );
    return ctxt;
}


void uiSeisSel::fillContext( Seis::GeomType geom, bool forread,
			     IOObjContext& ctxt )
{
    ctxt.forread = forread;
    if ( geom == Seis::Line )
	ctxt.toselect.allowtransls_ = ctxt.deftransl = "2D";
    else
	ctxt.deftransl = CBVSSeisTrcTranslator::translKey();
    if ( !forread )
	ctxt.toselect.allowtransls_ = ctxt.deftransl;
}


void uiSeisSel::newSelection( uiIOObjRetDlg* dlg )
{
    ((uiSeisSelDlg*)dlg)->fillPar( dlgiopar_ );
    setAttrNm( dlgiopar_.find( sKey::Attribute() ) );

    if ( seissetup_.selectcomp_ && !dlgiopar_.get(sKey::Component(), compnr_) )
	setCompNr( compnr_ );
}


IOObj* uiSeisSel::createEntry( const char* nm )
{
    if ( !Seis::is2D(seissetup_.geom_) || Seis::isPS(seissetup_.geom_) )
	return uiIOObjSel::createEntry( nm );

    CtxtIOObj newctio( inctio_.ctxt );
    newctio.setName( nm );
    newctio.fillObj();
    if ( !newctio.ioobj ) return 0;
    mDynamicCastGet(IOStream*,iostrm,newctio.ioobj)
    if ( !iostrm )
	return newctio.ioobj;

    iostrm->setTranslator( "2D" );
    iostrm->setExt( "2ds" );
    return iostrm;
}


void uiSeisSel::setAttrNm( const char* nm )
{
    attrnm_ = nm;
    if ( attrnm_.isEmpty() )
	dlgiopar_.removeWithKey( sKey::Attribute() );
    else
	dlgiopar_.set( sKey::Attribute(), nm );
    updateInput();
}


const char* uiSeisSel::userNameFromKey( const char* txt ) const
{
    if ( !txt || !*txt ) return "";

    LineKey lk( txt );
    curusrnm_ = uiIOObjSel::userNameFromKey( lk.lineName() );
    if ( is2D() )
	curusrnm_ = LineKey( curusrnm_, lk.attrName() );
    return curusrnm_.buf();
}


void uiSeisSel::setCompNr( int nr )
{
    compnr_ = nr;
    if ( mIsUdf(compnr_) )
	dlgiopar_.removeWithKey( sKey::Component() );
    else
	dlgiopar_.set( sKey::Component(), nr );
    updateInput();
}


const char* uiSeisSel::compNameFromKey( const char* txt ) const
{
    if ( !txt || !*txt ) return "";

    BufferString compnm;
    LineKey lk( txt );
    compnm = uiIOObjSel::userNameFromKey( lk.attrName() );
    if ( is2D() )
    {
	const char* ptr = "";
	ptr = strchr( compnm.buf(), '|' );
	if ( ptr )
	    { ptr++; mSkipBlanks(ptr); }
	else
	    ptr = "";

	compnm = BufferString( ptr );
    }
    return compnm.buf();
}


bool uiSeisSel::existingTyped() const
{
    bool containscompnm = false;
    const char* ptr = "";                                                   
    ptr = strchr( getInput(), '|' );                                      
    if ( ptr )
	containscompnm = true;
    return (!is2D() && !containscompnm) || isPS() ? uiIOObjSel::existingTyped()
	 : existingUsrName( LineKey(getInput()).lineName() );
}


bool uiSeisSel::fillPar( IOPar& iop ) const
{
    iop.merge( dlgiopar_ );
    return uiIOObjSel::fillPar( iop );
}


void uiSeisSel::usePar( const IOPar& iop )
{
    uiIOObjSel::usePar( iop );
    dlgiopar_.merge( iop );
    attrnm_ = iop.find( sKey::Attribute() );

    if ( seissetup_.selectcomp_ && !iop.get(sKey::Component(), compnr_) )
	compnr_ = 0;
}


void uiSeisSel::updateInput()
{
    BufferString ioobjkey;
    if ( workctio_.ioobj )
	ioobjkey = workctio_.ioobj->key();

    if ( workctio_.ctxt.forread )
	updateAttrNm();

    uiIOSelect::setInput( LineKey(ioobjkey,attrnm_).buf() );

    if ( seissetup_.selectcomp_ && !mIsUdf( compnr_ ) )
    {
	SeisTrcReader rdr( workctio_.ioobj );
	if ( !rdr.prepareWork(Seis::PreScan) ) return;
	SeisTrcTranslator* transl = rdr.seisTranslator();
	if ( !transl ) return;
	BufferStringSet compnms;
	transl->getComponentNames( compnms );
	if ( compnr_ >= compnms.size() || compnms.size()<2 ) return;

	BufferString text = userNameFromKey( LineKey(ioobjkey,attrnm_).buf() );
	text += "|";
	text += compnms.get( compnr_ );
	uiIOSelect::setInputText( text.buf() );
    }
}


void uiSeisSel::updateAttrNm()
{
    if ( is2D() && workctio_.ioobj )
    {
	SeisIOObjInfo seisinfo( workctio_.ioobj  );
	SeisIOObjInfo::Opts2D opt2d;
	opt2d.steerpol_ = seissetup_.steerpol_;
	opt2d.zdomky_ = seissetup_.zdomkey_;
	BufferStringSet attrnms;
	seisinfo.getAttribNames( attrnms, opt2d );
	if ( attrnm_.isEmpty() || !attrnms.isPresent(attrnm_) )
	{
	    const int attridx = attrnms.nearestMatch( "Seis" );
	    if ( attridx >=0 && !mIsUdf(attridx) )
		attrnm_ = attrnms.get( attridx );
	    if ( attrnm_.isEmpty() )
		attrnm_ = opt2d.steerpol_ == 1 ? sKey::Steering() : "Seis";
	}
    }
}


void uiSeisSel::commitSucceeded()
{
    if ( !othdombox_ || !othdombox_->isChecked() ) return;

    const ZDomain::Def* def = SI().zIsTime() ? &ZDomain::Depth()
					     : &ZDomain::Time();
    def->set( dlgiopar_ );
    if ( inctio_.ioobj )
    {
	def->set( inctio_.ioobj->pars() );
	IOM().commitChanges( *inctio_.ioobj );
    }
}


void uiSeisSel::processInput()
{
    obtainIOObj();
    attrnm_ = workctio_.ioobj ? LineKey( getInput() ).attrName() : "";
    if ( workctio_.ioobj || workctio_.ctxt.forread )
	updateInput();
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, workctio_, seissetup_ );
    dlg->usePar( dlgiopar_ );
    return dlg;
}
