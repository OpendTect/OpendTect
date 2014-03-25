/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseissel.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uimsg.h"

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


uiString uiSeisSelDlg::gtSelTxt( const uiSeisSel::Setup& setup, bool forread )
{
    if ( !setup.seltxt_.isEmpty() )
	return setup.seltxt_;

    switch ( setup.geom_ )
    {
    case Seis::Vol:
	return forread ? tr("Input Cube") : tr("Output Cube");
    case Seis::Line:
	return forread ?  tr("Input Data Set") : tr("Output Data Set");
    default:
	return forread ? tr("Input Data Store") : tr("Output Data Store");
    }
}


static void adaptCtxt( const IOObjContext& ct, const uiSeisSel::Setup& su,
			bool chgtol )
{
    IOObjContext& ctxt = const_cast<IOObjContext&>( ct );

    if ( su.geom_ == Seis::Line )
	ctxt.toselect.allowtransls_ = "TwoD DataSet";
    else
    {
	if ( su.steerpol_ == uiSeisSel::Setup::NoSteering )
	    ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );
	else if ( su.steerpol_ == uiSeisSel::Setup::OnlySteering )
	    ctxt.toselect.require_.set( sKey::Type(), sKey::Steering() );
    }

    if ( ctxt.deftransl.isEmpty() )
	ctxt.deftransl = su.geom_ == Seis::Line ? "TwoD DataSet"
				: CBVSSeisTrcTranslator::translKey();

    if ( !ctxt.forread )
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
    : uiIOObjSelDlg(p,getDlgCtio(c,sssu),"",false,sssu.allowsetsurvdefault_)
    , compfld_(0)
    , steerpol_(sssu.steerpol_)
    , zdomainkey_(sssu.zdomkey_)
{
    setSurveyDefaultSubsel( sssu.survdefsubsel_ );

    const bool is2d = Seis::is2D( sssu.geom_ );
    const bool isps = Seis::isPS( sssu.geom_ );

    if ( is2d )
    {
	selgrp_->getTopGroup()->display( true, true );
	selgrp_->getNameField()->display( true, true );
	selgrp_->getListField()->display( true, true );

	if ( c.ioobj )
	{
	    TypeSet<MultiID> selmids;
	    selmids += c.ioobj->key();
	    selgrp_->setSelected( selmids );
	}
    }

    uiString titletxt( uiStrings::sSelect(true,false) );
    if ( !sssu.seltxt_.isEmpty() )
	titletxt = titletxt.arg( sssu.seltxt_ );
    else
	titletxt = titletxt.arg( isps
                ? tr("Data Store")
                : (is2d ? tr("Dataset") : tr("Cube")) );
    setTitleText( titletxt );

    uiGroup* topgrp = selgrp_->getTopGroup();

    if ( sssu.selattr_ && is2d && !isps )
	selgrp_->getListField()->selectionChanged.notify(
					    mCB(this,uiSeisSelDlg,attrNmSel) );
    else
	selgrp_->getListField()->selectionChanged.notify(
					    mCB(this,uiSeisSelDlg,entrySel) );

    if ( !selgrp_->getCtxtIOObj().ctxt.forread && Seis::is2D(sssu.geom_) )
	selgrp_->setConfirmOverwrite( false );
    entrySel(0);

    if ( selgrp_->getCtxtIOObj().ctxt.forread && sssu.selectcomp_ )
    {
	compfld_ = new uiLabeledComboBox( selgrp_, "Component", "Compfld" );
	compfld_->attach( alignedBelow, topgrp );

	entrySel(0);
    }
}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    // ioobj should already be filled by base class
    const IOObj* ioobj = ioObj();
    if ( !ioobj || ( !compfld_ ) )
	return;

    IOObjContext ctxt = selgrp_->getCtxtIOObj().ctxt;

    if ( ctxt.forread && compfld_ )
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


void uiSeisSelDlg::attrNmSel( CallBacker* )
{
    selgrp_->getNameField()->setText( selgrp_->getListField()->getText() );
}


const char* uiSeisSelDlg::getDataType()
{
    if ( steerpol_ )
	return steerpol_ == uiSeisSel::Setup::NoSteering
			  ? 0 : sKey::Steering().str();
    const IOObj* ioobj = ioObj();
    if ( !ioobj ) return 0;
    const char* res = ioobj->pars().find( sKey::Type() );
    return res;
}


void uiSeisSelDlg::fillPar( IOPar& iopar ) const
{
    uiIOObjSelDlg::fillPar( iopar );
    const IOObj* ioobj = ioObj();
    if ( !ioobj )
	return;

    SeisIOObjInfo oinf( *ioobj );
    const bool is2d = oinf.is2D();

    if ( is2d )
    {
	BufferString dsnm( selgrp_->getNameField()->text() );
	const int nroccuer = dsnm.count( '|' );
	dsnm.replace( '|', '_' );
	if( nroccuer )
	{
	    BufferString msg( "Invalid charactor  '|' " );
	    msg.add( " found in attribute name. " )
	       .add( "It will be renamed to: '" )
	       .add( dsnm.buf() ).add("'." )
	       .add( "\nDo you want to continue?" );
	    if( !uiMSG().askGoOn( msg.buf() ) )
		return;
	}

	iopar.set( sKey::DataSet(), dsnm );
    }

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

    if ( compfld_ )
	entrySel(0);

    if ( iopar.find( sKey::DataSet() ) )
    {
	const char* seldatasetnm = iopar.find( sKey::DataSet() );
	if ( seldatasetnm )
	{
	    uiListBox* listfld = selgrp_->getListField();
	    if ( listfld ) listfld->setCurrentItem( seldatasetnm );
	    uiGenInput* nmfld = selgrp_->getNameField();
	    if (nmfld ) nmfld->setText( seldatasetnm );
	}
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
    if ( is2D() )
	inpBox()->setReadOnly( true );

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
    if ( is2D() )
	inpBox()->setReadOnly( true );

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
    ret.seltxt_ = uiSeisSelDlg::gtSelTxt( su, forread );
    ret.filldef( su.allowsetdefault_ );
    return ret;
}


CtxtIOObj* uiSeisSel::mkCtxtIOObj( Seis::GeomType gt, bool forread )
{
    const bool is2d = Seis::is2D( gt );

    CtxtIOObj* ret;
    if ( Seis::isPS(gt) )
	ret = is2d ? mMkCtxtIOObj(SeisPS2D) : mMkCtxtIOObj(SeisPS3D);
    else
	ret = mMkCtxtIOObj(SeisTrc);

    ret->ctxt.forread = forread;
    return ret;
}


const char* uiSeisSel::getDefaultKey( Seis::GeomType gt ) const
{
    const bool is2d = Seis::is2D( gt );
    return IOPar::compKey( sKey::Default(),
	is2d ? SeisTrcTranslatorGroup::sKeyDefault2D()
	     : SeisTrcTranslatorGroup::sKeyDefault3D() );
}


void uiSeisSel::fillDefault()
{
    if ( !setup_.filldef_ || workctio_.ioobj || !workctio_.ctxt.forread )
	return;

    workctio_.destroyAll();
    if ( Seis::isPS(seissetup_.geom_) )
	workctio_.fillDefault();
    else
	workctio_.fillDefaultWithKey( getDefaultKey(seissetup_.geom_) );
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
	ctxt.toselect.allowtransls_ = ctxt.deftransl = "TwoD DataSet";
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

    iostrm->setTranslator( "TwoD DataSet" );
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
	ptr = firstOcc( compnm.buf(), '|' );
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
    ptr = firstOcc( getInput(), '|' );
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

    if ( !ioobjkey.isEmpty() )
    {
	if ( seissetup_.selattr_ )
	    uiIOSelect::setInput( LineKey(ioobjkey,attrnm_).buf() );
	else
	    uiIOSelect::setInput( ioobjkey );
    }

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
    if ( !seissetup_.selattr_ )
    {
	attrnm_ = "";
	return;
    }

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
    if ( !workctio_.ioobj && !workctio_.ctxt.forread )
	return;

    setAttrNm( workctio_.ioobj ? LineKey( getInput() ).attrName() : "" );
    uiIOObjSel::fillPar( dlgiopar_ );
    updateInput();
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, workctio_, seissetup_ );
    dlg->usePar( dlgiopar_ );
    return dlg;
}

// uiSteerCubeSel

static uiSeisSel::Setup mkSeisSelSetupForSteering( bool is2d, bool forread,
						   const char* txt )
{
    uiSeisSel::Setup sssu( is2d, false );
    sssu.selattr( is2d ).wantSteering().seltxt( txt );
    if ( !forread && is2d ) sssu.allowlinesetsel( false );
    return sssu;
}


uiSteerCubeSel::uiSteerCubeSel( uiParent* p, bool is2d, bool forread,
				const char* txt )
	: uiSeisSel(p,uiSeisSel::ioContext(is2d?Seis::Line:Seis::Vol,forread),
		    mkSeisSelSetupForSteering(is2d,forread,txt))
{
}


const char* uiSteerCubeSel::getDefaultKey( Seis::GeomType gt ) const
{
    BufferString defkey = uiSeisSel::getDefaultKey( gt );
    return IOPar::compKey( defkey, sKey::Steering() );
}

