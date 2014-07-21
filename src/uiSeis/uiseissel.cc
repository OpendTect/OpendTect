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
#include "seis2dlineio.h"
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


static void adaptCtxt4Steering( const IOObjContext& ct,
				const uiSeisSel::Setup& su )
{
    IOObjContext& ctxt = const_cast<IOObjContext&>( ct );

    if ( su.steerpol_ == uiSeisSel::Setup::NoSteering )
	ctxt.toselect.dontallow_.set( sKey::Type(), sKey::Steering() );
    else if ( su.steerpol_ == uiSeisSel::Setup::OnlySteering )
    {
	ctxt.toselect.require_.set( sKey::Type(), sKey::Steering() );
	ctxt.fixTranslator( CBVSSeisTrcTranslator::translKey() );
    }
}


static const CtxtIOObj& getDlgCtio( const CtxtIOObj& c,
				    const uiSeisSel::Setup& s )
{
    adaptCtxt4Steering( c.ctxt, s );
    return c;
}



uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const uiSeisSel::Setup& sssu )
    : uiIOObjSelDlg(p,getDlgCtio(c,sssu),uiStrings::sEmptyString(),false,
                                 sssu.allowsetsurvdefault_)
    , compfld_(0)
    , steerpol_(sssu.steerpol_)
    , zdomainkey_(sssu.zdomkey_)
{
    setSurveyDefaultSubsel( sssu.survdefsubsel_ );

    const bool is2d = Seis::is2D( sssu.geom_ );
    const bool isps = Seis::isPS( sssu.geom_ );

    if ( is2d )
    {
	if ( selgrp_->getTopGroup() )
	    selgrp_->getTopGroup()->display( true, true );
	if ( selgrp_->getNameField() )
	    selgrp_->getNameField()->display( true, true );
	if ( selgrp_->getListField() )
	    selgrp_->getListField()->display( true, true );

	if ( c.ioobj )
	{
	    TypeSet<MultiID> selmids;
	    selmids += c.ioobj->key();
	    selgrp_->setChosen( selmids );
	}
    }

    uiString titletxt( tr("Select %1") );
    if ( !sssu.seltxt_.isEmpty() )
	titletxt = titletxt.arg( sssu.seltxt_ );
    else
	titletxt = titletxt.arg( isps
                ? tr("Data Store")
                : (is2d ? tr("Dataset") : tr("Cube")) );
    setTitleText( titletxt );

    uiGroup* topgrp = selgrp_->getTopGroup();
    selgrp_->getListField()->selectionChanged.notify(
					    mCB(this,uiSeisSelDlg,entrySel) );

    if ( !selgrp_->getCtxtIOObj().ctxt.forread && Seis::is2D(sssu.geom_) )
	selgrp_->setConfirmOverwrite( false );
    entrySel(0);

    if ( selgrp_->getCtxtIOObj().ctxt.forread && sssu.selectcomp_ )
    {
	compfld_ = new uiLabeledComboBox( selgrp_, tr("Component"),
                                          "Compfld" );
	compfld_->attach( alignedBelow, topgrp );

	entrySel(0);
    }
}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    if ( !compfld_ )
	return;

    const IOObjContext& ctxt = selgrp_->getCtxtIOObj().ctxt;
    if ( !ctxt.forread )
	return;

    const IOObj* ioobj = ioObj(); // do NOT call this function when for write
    if ( !ioobj )
	return;

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
    adaptCtxt4Steering( c.ctxt, s );
    return c;
}


static const IOObjContext& getIOObjCtxt( const IOObjContext& c,
					 const uiSeisSel::Setup& s )
{
    adaptCtxt4Steering( c, s );
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
}


void uiSeisSel::mkOthDomBox()
{
    if ( !inctio_.ctxt.forread && seissetup_.enabotherdomain_ )
    {
	othdombox_ = new uiCheckBox( this, SI().zIsTime() ? uiStrings::sDepth()
                                                          : uiStrings::sTime());
	othdombox_->attach( rightOf, endObj(false) );
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


IOObjContext uiSeisSel::ioContext( Seis::GeomType gt, bool forread )
{
    PtrMan<IOObjContext> newctxt = Seis::getIOObjContext( gt, forread );
    return IOObjContext( *newctxt );
}


void uiSeisSel::newSelection( uiIOObjRetDlg* dlg )
{
    ((uiSeisSelDlg*)dlg)->fillPar( dlgiopar_ );
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

    iostrm->setTranslator( TwoDDataSeisTrcTranslator::translKey() );
    return iostrm;
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

    LineKey lk( txt );
    return uiIOObjSel::userNameFromKey( lk.attrName() );
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

    if ( seissetup_.selectcomp_ && !iop.get(sKey::Component(), compnr_) )
	compnr_ = 0;
}


void uiSeisSel::updateInput()
{
    BufferString ioobjkey;
    if ( workctio_.ioobj )
	ioobjkey = workctio_.ioobj->key();

    if ( !ioobjkey.isEmpty() )
	uiIOSelect::setInput( ioobjkey );

    if ( seissetup_.selectcomp_ && !mIsUdf( compnr_ ) )
    {
	SeisTrcReader rdr( workctio_.ioobj );
	if ( !rdr.prepareWork(Seis::PreScan) ) return;
	SeisTrcTranslator* transl = rdr.seisTranslator();
	if ( !transl ) return;
	BufferStringSet compnms;
	transl->getComponentNames( compnms );
	if ( compnr_ >= compnms.size() || compnms.size()<2 ) return;

	BufferString text = userNameFromKey( ioobjkey );
	text += "|";
	text += compnms.get( compnr_ );
	uiIOSelect::setInputText( text.buf() );
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
    sssu.wantSteering().seltxt( txt );
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

