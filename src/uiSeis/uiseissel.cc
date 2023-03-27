/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseissel.h"
#include "uiseisposprovgroup.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjselwritetransl.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uiselsurvranges.h"
#include "uimsg.h"
#include "uistrings.h"

#include "zdomain.h"
#include "ctxtioobj.h"
#include "trckeyzsampling.h"
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

    uiString datatype = Seis::dataName(setup.geom_, setup.explprepost_ );

    return forread
	? uiStrings::phrInput( datatype )
	: uiStrings::phrOutput( datatype );
}


static IOObjContext adaptCtxt4Steering( const IOObjContext& ct,
				const uiSeisSel::Setup& su )
{
    IOObjContext ctxt( ct );
    if ( su.steerpol_ == uiSeisSel::Setup::NoSteering )
	ctxt.toselect_.dontallow_.addVal( sKey::Type(), sKey::Steering() );
    else if ( su.steerpol_ == uiSeisSel::Setup::OnlySteering )
	ctxt.toselect_.require_.set( sKey::Type(), sKey::Steering() );

    return ctxt;
}


static CtxtIOObj adaptCtio4Steering( const CtxtIOObj& ct,
				const uiSeisSel::Setup& su )
{
    CtxtIOObj ctio( ct );
    ctio.ctxt_ = adaptCtxt4Steering( ctio.ctxt_, su );
    return ctio;
}


static uiIOObjSelDlg::Setup getSelDlgSU( const uiSeisSel::Setup& sssu )
{
    uiIOObjSelDlg::Setup sdsu;
    sdsu.allowsetsurvdefault( sssu.allowsetsurvdefault_ );
    sdsu.withwriteopts( sssu.withwriteopts_ );
    return sdsu;
}


uiSeisSelDlg::uiSeisSelDlg( uiParent* p, const CtxtIOObj& c,
			    const uiSeisSel::Setup& sssu )
    : uiIOObjSelDlg(p,getSelDlgSU(sssu),adaptCtio4Steering(c,sssu))
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

	if ( c.ioobj_)
	{
	    TypeSet<MultiID> selmids;
	    selmids += c.ioobj_->key();
	    selgrp_->setChosen( selmids );
	}
    }

    uiString titletxt( tr("Select %1") );
    if ( !sssu.seltxt_.isEmpty() )
	titletxt = titletxt.arg( sssu.seltxt_ );
    else
	titletxt = titletxt.arg( isps
		? tr("Data Store")
		: (is2d ? tr("Dataset") : uiStrings::sVolume()) );
    setTitleText( titletxt );

    uiGroup* topgrp = selgrp_->getTopGroup();
    selgrp_->getListField()->selectionChanged.notify(
					    mCB(this,uiSeisSelDlg,entrySel) );

    if ( !selgrp_->getCtxtIOObj().ctxt_.forread_ && Seis::is2D(sssu.geom_) )
	selgrp_->setConfirmOverwrite( false );

    if ( selgrp_->getCtxtIOObj().ctxt_.forread_ && sssu.selectcomp_ )
    {
	compfld_ = new uiLabeledComboBox( selgrp_, uiStrings::sComponent(),
					  "Compfld" );
	compfld_->box()->setHSzPol( uiObject::WideVar );
	compfld_->attach( alignedBelow, topgrp );
	if ( selgrp_->getCtxtIOObj().ioobj_ )
	    entrySel( nullptr );
    }
}


uiSeisSelDlg::~uiSeisSelDlg()
{}


void uiSeisSelDlg::entrySel( CallBacker* )
{
    if ( !compfld_ )
	return;

    if ( selgrp_->getListField()->currentItem() < 0 )
    {
	compfld_->display( false );
	return;
    }

    const IOObjContext& ctxt = selgrp_->getCtxtIOObj().ctxt_;
    if ( !ctxt.forread_ )
	return;

    const IOObj* ioobj = ioObj(); // do NOT call this function when for write
    if ( !ioobj )
	return;

    compfld_->box()->setCurrentItem( 0 );

    BufferStringSet compnms;
    getComponentNames( compnms );

    compfld_->box()->setEmpty();
    compfld_->box()->addItems( compnms );
    compfld_->display( compnms.size() > 1 );
}


BufferString uiSeisSelDlg::getDataType()
{
    BufferString res;
    if ( steerpol_ )
	return steerpol_ == uiSeisSel::Setup::NoSteering
			  ? res : BufferString( sKey::Steering() );
    const IOObj* ioobj = ioObj();
    if ( ioobj )
	res = ioobj->pars().find( sKey::Type() );

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
    {
	if ( selgrp_->getCtxtIOObj().ioobj_ )
	    entrySel( nullptr );

	int selcompnr = mUdf(int);
	if ( iopar.get(sKey::Component(),selcompnr) && !mIsUdf(selcompnr) )
	{
	    BufferStringSet compnms;
	    getComponentNames( compnms );
	    if ( selcompnr >= compnms.size() )
		return;

	    compfld_->box()->setText( compnms.get(selcompnr).buf() );
	}
    }
}


void uiSeisSelDlg::getComponentNames( BufferStringSet& compnms ) const
{
    compnms.erase();
    const IOObj* ioobj = ioObj();
    if ( !ioobj )
	return;

    SeisIOObjInfo info( ioobj );
    info.getComponentNames( compnms );
}


static IOObjContext getIOObjCtxt( const IOObjContext& c,
				  const uiSeisSel::Setup& s )
{
    return adaptCtxt4Steering( c, s );
}


uiSeisSel::uiSeisSel( uiParent* p, const IOObjContext& ctxt,
		      const uiSeisSel::Setup& su )
    : uiIOObjSel(p,getIOObjCtxt(ctxt,su),mkSetupWithCtxt(su,ctxt))
    , seissetup_(mkSetupWithCtxt(su,ctxt))
    , othdombox_(nullptr)
    , compnr_(0)
{
    workctio_.ctxt_ = inctio_.ctxt_;
    if ( !ctxt.forread_ && Seis::is2D(seissetup_.geom_) )
	seissetup_.confirmoverwr_ = setup_.confirmoverwr_ = false;

    mkOthDomBox();
}


void uiSeisSel::mkOthDomBox()
{
    if ( !inctio_.ctxt_.forread_ && seissetup_.enabotherdomain_ )
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


uiSeisSel::Setup uiSeisSel::mkSetupWithCtxt( const uiSeisSel::Setup& su,
					    const IOObjContext& ctxt )
{
    uiSeisSel::Setup ret( su );
    ret.seltxt_ = uiSeisSelDlg::gtSelTxt( su,  ctxt.forread_ );
    ret.filldef( su.allowsetdefault_ );
    ret.withwriteopts_ = !ctxt.forread_;
    if ( ctxt.trgroup_ && !ctxt.forread_ &&
	su.steerpol_ == Setup::OnlySteering )
    {
	const TranslatorGroup& trgrp = *ctxt.trgroup_;
	const ObjectSet<const Translator>& alltrs = trgrp.templates();
	for ( const auto* transl : alltrs )
	{
	    mDynamicCastGet(const SeisTrcTranslator*,seistr,transl);
	    if ( seistr && !seistr->supportsMultiCompTrc() )
	    {
		const BufferString nm = transl->typeName();
		ret.trsnotallwed_.addIfNew( transl->typeName() );
	    }
	}
    }

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
    if ( !setup_.filldef_ || workctio_.ioobj_ || !workctio_.ctxt_.forread_ )
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


const char* uiSeisSel::userNameFromKey( const char* txt ) const
{
    if ( !txt || !*txt )
	return "";

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
    if ( !txt || !*txt )
	return "";

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

    updateInput();
}


void uiSeisSel::updateInput()
{
    MultiID ioobjkey = MultiID::udf();
    if ( workctio_.ioobj_ )
    {
	ioobjkey = workctio_.ioobj_->key();
	if ( wrtrselfld_ )
	    wrtrselfld_->use( *workctio_.ioobj_ );
    }

    if ( !ioobjkey.isUdf() )
	uiIOSelect::setInput( ioobjkey.toString() );

    if ( seissetup_.selectcomp_ && !mIsUdf(compnr_) )
    {
	SeisIOObjInfo info( ioobjkey );
	BufferStringSet compnms;
	info.getComponentNames( compnms );
	if ( !compnms.validIdx(compnr_) || compnms.size()<2 )
	    return;

	BufferString text = userNameFromKey( ioobjkey.toString() );
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
    if ( inctio_.ioobj_ )
    {
	def->set( inctio_.ioobj_->pars() );
	IOM().commitChanges( *inctio_.ioobj_ );
    }
}


void uiSeisSel::updateOutputOpts( bool issteering )
{
    BufferStringSet transntallowed;
    if ( !issteering )
    {
	wrtrselfld_->updateTransFld( transntallowed );
	return;
    }

    CtxtIOObj& ctxt = ctxtIOObj();
    const TranslatorGroup& trgrp = *ctxt.ctxt_.trgroup_;
    const ObjectSet<const Translator>& alltrs = trgrp.templates();
    for ( const auto* transl : alltrs )
    {
	mDynamicCastGet(const SeisTrcTranslator*,seistr,transl);
	if ( seistr && !seistr->supportsMultiCompTrc() )
	    transntallowed.add( transl->typeName() );
    }

    wrtrselfld_->updateTransFld( transntallowed );
}


void uiSeisSel::processInput()
{
    obtainIOObj();
    if ( !workctio_.ioobj_ && !workctio_.ctxt_.forread_ )
	return;

    uiIOObjSel::fillPar( dlgiopar_ );
    updateInput();
}


uiIOObjRetDlg* uiSeisSel::mkDlg()
{
    uiSeisSelDlg* dlg = new uiSeisSelDlg( this, workctio_, seissetup_ );
    dlg->usePar( dlgiopar_ );
    uiIOObjSelGrp* selgrp = dlg->selGrp();
    if ( selgrp )
    {
	selgrp->setConfirmOverwrite( false );
	if ( wrtrselfld_ )
	    selgrp->setDefTranslator( wrtrselfld_->selectedTranslator() );
    }

    return dlg;
}

// uiSteerCubeSel

static uiSeisSel::Setup mkSeisSelSetupForSteering( bool is2d, bool forread,
						   const uiString& txt )
{
    uiSeisSel::Setup sssu( is2d, false );
    sssu.wantSteering().seltxt( txt );
    sssu.withwriteopts( true ).withinserters( false );
    return sssu;
}


uiSteerCubeSel::uiSteerCubeSel( uiParent* p, bool is2d, bool forread,
				const uiString& txt )
	: uiSeisSel(p,uiSeisSel::ioContext(is2d?Seis::Line:Seis::Vol,forread),
		    mkSeisSelSetupForSteering(is2d,forread,txt))
{
}


uiSteerCubeSel::~uiSteerCubeSel()
{}


const char* uiSteerCubeSel::getDefaultKey( Seis::GeomType gt ) const
{
    BufferString defkey = uiSeisSel::getDefaultKey( gt );
    return IOPar::compKey( defkey, sKey::Steering() );
}


uiSeisPosProvGroup::uiSeisPosProvGroup( uiParent* p,
					  const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , zrgfld_(0)
{
    uiSeisSel::Setup ssu( Seis::Vol );
    ssu.seltxt( tr("Cube for positions") );
    seissel_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,true), ssu );

    if ( su.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, su.withstep_, false, 0, su.zdomkey_ );
	zrgfld_->attach( alignedBelow, seissel_ );
    }

    setHAlignObj( seissel_ );
}


uiSeisPosProvGroup::~uiSeisPosProvGroup()
{}


#define mGetSeis3DKey(k) IOPar::compKey(sKeyType(),k)

void uiSeisPosProvGroup::usePar( const IOPar& iop )
{
    PtrMan<IOPar> subiop = iop.subselect( sKeyType() );
    if ( !subiop || subiop->isEmpty() )
	return;

    seissel_->usePar( *subiop );
    if ( zrgfld_ )
    {
	StepInterval<float> zsamp;
	if ( subiop->get(sKey::ZRange(),zsamp) )
	    zrgfld_->setRange( zsamp );
    }
}


bool uiSeisPosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKeyType() );
    IOPar subiop;
    seissel_->fillPar( subiop );

    if ( zrgfld_ )
	subiop.set( sKey::ZRange(), zrgfld_->getRange() );

    iop.mergeComp( subiop, sKeyType() );
    return true;
}


void uiSeisPosProvGroup::getSummary( BufferString& txt ) const
{
    txt.set( "From 3D Seismic" );
    const IOObj* ioobj = seissel_->ioobj( true );
    if ( ioobj )
	txt.add( " '" ).add( ioobj->name() ).add( "." );
}


void uiSeisPosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKeyType(),
					  tr("Seismic Cube Positions") );
}
