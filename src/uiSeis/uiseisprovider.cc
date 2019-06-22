/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		March 2019
________________________________________________________________________

-*/


#include "uiseisprovider.h"

#include "dbdir.h"
#include "ioobj.h"
#include "ioobjctxt.h"
#include "keystrs.h"
#include "seisioobjinfo.h"
#include "seisrangeseldata.h"
#include "seisprovider.h"
#include "survinfo.h"
#include "survsubsel.h"
#include "surveydisklocation.h"

#include "uilistbox.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseisseldata.h"
#include "uistrings.h"


#define mInpSelTxt(nm) uiStrings::phrInput( nm )
#define mInpSelTxtGen() mInpSelTxt( uiStrings::sSeismicData() )


uiSeisProvider::Setup::Setup()
    : sdl_(*new SurveyDiskLocation)
{
    gts_.add( Seis::Vol ).add( Seis::Line )
	.add( Seis::VolPS ).add( Seis::LinePS );
    seltxt_.set( mInpSelTxtGen() );
}


uiSeisProvider::Setup::Setup( GeomType gt )
    : sdl_(*new SurveyDiskLocation)
{
    gts_.add( gt );
    seltxt_.set( mInpSelTxt(Seis::dataName(gt)) );
}


uiSeisProvider::Setup::Setup( GeomType gt1, GeomType gt2 )
    : sdl_(*new SurveyDiskLocation)
{
    gts_.add( gt1 ).add( gt2 );
    seltxt_.set( mInpSelTxtGen() );
}


uiSeisProvider::Setup::Setup( GeomType gt1, GeomType gt2, GeomType gt3 )
    : sdl_(*new SurveyDiskLocation)
{
    gts_.add( gt1 ).add( gt2 ).add( gt3 );
    seltxt_.set( mInpSelTxtGen() );
}


uiSeisProvider::Setup::Setup( PSPol pspol )
    : sdl_(*new SurveyDiskLocation)
{
    if ( pspol == OnlyPS )
    {
	gts_.add( Seis::VolPS ).add( Seis::LinePS );
	seltxt_.set( mInpSelTxt(uiStrings::sPreStackData()) );
    }
    else
    {
	gts_.add( Seis::Vol ).add( Seis::Line );
	seltxt_.set( mInpSelTxtGen() );
    }
}


uiSeisProvider::Setup::Setup( const Setup& oth )
    : sdl_(*new SurveyDiskLocation(oth.sdl_))
{
    *this = oth;
}


uiSeisProvider::Setup& uiSeisProvider::Setup::operator =( const Setup& oth )
{
    if ( this != &oth )
    {
	gts_ = oth.gts_;
	sdl_ = oth.sdl_;
	style_ = oth.style_;
	subselpol_ = oth.subselpol_;
	steerpol_ = oth.steerpol_;
	compselpol_ = oth.compselpol_;
	optional_ = oth.optional_;
	seltxt_ = oth.seltxt_;
	zdominf_ = oth.zdominf_;
    }
    return *this;
}


uiSeisProvider::Setup::~Setup()
{
    delete &sdl_;
}


namespace Seis
{

//!\brief interface allowing future add of VolProc and/or Attrib

class ProviderInfo
{
public:

    typedef od_int64	alt_sortkey_type;
    mUseType( Survey,	GeomSubSel );

    virtual			~ProviderInfo()		{}

    virtual BufferString	key() const		= 0;
    virtual BufferString	name() const		= 0;
    virtual BufferStringSet	componentNames() const	= 0;
    virtual BufferString	iconID() const		= 0;
    virtual bool		isDefault() const	= 0;
    virtual alt_sortkey_type	altSortKey() const	{ return 0; }

};


//!\brief interface allowing future add of VolProc and/or Attrib

class ProviderInfoList : public ObjectSet<ProviderInfo>
{
public:

    mUseType( Seis,	GeomType );

    virtual const char*	provType() const		= 0;
    virtual GeomType	geomType() const		= 0;
    virtual uiString	altSortKeyDispStr() const	= 0;

~ProviderInfoList()
{
    deepErase( *this );
}

idx_type addDispNames( BufferStringSet& nms ) const
{
    idx_type defitm = -1;
    for ( auto inf : *this )
    {
	nms.add( inf->name() );
	if ( defitm < 0 && inf->isDefault() )
	    defitm = indexOf( inf );
    }
    return defitm;
}

void sortEntries( bool byname )
{
    BufferStringSet nms;
    TypeSet<od_int64> modiftms;

    for ( auto inf : *this )
    {
	if ( byname )
	    nms.add( inf->name() );
	else
	{
	    const auto mtm = inf->altSortKey();
	    modiftms.add( mIsUdf(mtm) ? 0 : mtm );
	}
    }

    idx_type* idxs = byname ? nms.getSortIndexes()
			    : getSortIndexes( modiftms, false );
    useIndexes( idxs );
    delete [] idxs;
}

};

class ObjectSummaryProviderInfo : public ProviderInfo
{
public:

ObjectSummaryProviderInfo( const IOObj& ioobj )
    : summ_(ioobj)
{
}

~ObjectSummaryProviderInfo()
{
    delete survsubsel_;
}

BufferString key() const override
{ return toString(summ_.key()); }
BufferString name() const override
{ return summ_.name(); }
BufferStringSet componentNames() const override
{ return summ_.compNames(); }
BufferString iconID() const override
{ return summ_.ioObjInfo().iconID(); }
alt_sortkey_type altSortKey() const override
{ return summ_.mainFileModifTime(); }
bool isDefault() const
{
    const auto* ioobj = summ_.ioObjInfo().ioObj();
    return ioobj ? IOObj::isSurveyDefault(ioobj->key()) : false;
}

    const ObjectSummary	summ_;
    mutable GeomSubSel*	survsubsel_	= 0;

};

class ObjectSummaryProviderInfoList : public ProviderInfoList
{
public:


ObjectSummaryProviderInfoList( GeomType gt )
    : gt_(gt)
{}
const char* provType() const override
{ return "Stored"; }
GeomType geomType() const override
{ return gt_; }
uiString altSortKeyDispStr() const override
{ return uiStrings::sTimeSort(); }

    const GeomType	gt_;

};

} // namespace Seis




class uiSeisProviderData : public ObjectSet<Seis::ProviderInfoList>
{
public:

    mUseType( Seis,		GeomType );
    mUseType( uiSeisProvider,	Setup );

uiSeisProviderData( const Setup& su )
{
    for ( auto gt : su.gts_ )
	addGT( gt, su );
}

~uiSeisProviderData()
{
    deepErase( *this );
}

void addGT( GeomType gt, const Setup& su )
{
    // may in future have to add VolProc and Attrib stuff too

    const IOObjContext ctxt( uiSeisSel::ioContext(gt,true) );
    DBDirEntryList delst( ctxt, su.sdl_ );
    auto* lst = new Seis::ObjectSummaryProviderInfoList( gt );
    add( lst );
    for ( auto idx=0; idx<delst.size(); idx++ )
    {
	const IOObj& ioobj = delst.ioobj( idx );
	if ( !ioobj.isUserSelectable()
	|| (su.zdominf_ && !su.zdominf_->isCompatibleWith(ioobj.pars())) )
	    continue;

	auto* inf = new Seis::ObjectSummaryProviderInfo( ioobj );
	if ( inf->summ_.isOK() )
	    lst->add( inf );
	else
	    delete inf;
    }
}

void sortEntries( bool byname )
{
    for ( auto lst : *this )
	lst->sortEntries( byname );
}

};


uiSeisProvider::uiSeisProvider( uiParent* p, const Setup& su )
    : uiGroup(p,"Seismic Provider Data Selector")
    , setup_(su)
    , data_(*new uiSeisProviderData(su))
    , gtprov_(*new uiSeisProvGTProvider(*this))
    , selectionChanged(this)
    , geomTypeChanged(this)
{
    data_.sortEntries( true );
    maingrp_ = new uiGroup( this, "Main group" );

    if ( setup_.optional_ )
	optcheck_ = new uiCheckBox( this, setup_.seltxt_ );
    else if ( setup_.style_ != Compact && !setup_.seltxt_.isEmpty() )
	lbl_ = new uiLabel( maingrp_, setup_.seltxt_ );

    if ( setup_.gts_.size() > 1 )
    {
	uiStringSet gtstrs;
	for ( auto gt : setup_.gts_ )
	    gtstrs.add( Seis::dataName(gt) );
	geomtypebox_ = new uiComboBox( maingrp_, "GeomType box" );
	geomtypebox_->addItems( gtstrs );
    }

    if ( setup_.style_ < PickOne )
	itmbox_ = new uiComboBox( maingrp_, "Selection" );

    if ( setup_.compselpol_ != AllComps )
    {
	const char* fldnm = "Component";
	if ( setup_.style_ != PickOne )
	    compbox_ = new uiComboBox( maingrp_, fldnm );
	else
	{
	    uiListBox::Setup lbsu( setup_.compselpol_ == OneComp
			    ? OD::ChooseOnlyOne : OD::ChooseAtLeastOne );
	    complist_ = new uiListBox( maingrp_, lbsu, fldnm );
	}
    }

    if ( setup_.style_ == PickOne )
    {
	uiListBox::Setup lbsu( setup_.style_ == PickOne
		    ? OD::ChooseOnlyOne : OD::ChooseAtLeastOne );
	itmlist_ = new uiListBox( maingrp_, lbsu, "Selection" );
    }
    else if ( setup_.style_ == SingleLine )
	selbut_ = uiButton::getStd( maingrp_, OD::Select,
				    mCB(this,uiSeisProvider,selInpCB), true );

    if ( setup_.subselpol_ != NoSubSel )
	seldatafld_ = new uiSeisSelData( maingrp_ );

    attachFlds();

    mAttachCB( postFinalise(), uiSeisProvider::initGrp );
}


uiSeisProvider::~uiSeisProvider()
{
    delete fixedseldata_;
    delete &data_;
    delete &gtprov_;
}


void uiSeisProvider::initGrp( CallBacker* )
{
    fillUi();

    if ( !usrsetkey_.isEmpty() )
	set( DBKey(usrsetkey_) );
    else
    {
	const auto* info = curInfo();
	if ( info )
	    set( DBKey(info->key()) );
    }

    if ( geomtypebox_ )
	mAttachCB( geomtypebox_->selectionChanged, uiSeisProvider::gtChgCB );
    if ( itmbox_ )
	mAttachCB( itmbox_->selectionChanged, uiSeisProvider::selChgCB );
    if ( itmlist_ )
	mAttachCB( itmlist_->selectionChanged, uiSeisProvider::selChgCB );
    if ( optcheck_ )
	mAttachCB( optcheck_->activated, uiSeisProvider::optChgCB );
}


bool uiSeisProvider::isActive() const
{
    return !optcheck_ || optcheck_->isChecked();
}


void uiSeisProvider::gtChgCB( CallBacker* )
{
    fillUi();
    geomTypeChanged.trigger();
}


void uiSeisProvider::optChgCB( CallBacker* )
{
    updUi();
}


void uiSeisProvider::selInpCB( CallBacker* )
{
    const auto gt = geomType();
    const bool docomps = setup_.compselpol_ == OneComp;

    CtxtIOObj ctio( uiSeisSel::ioContext(gt,true) );
    uiSeisSel::Setup sssu( gt );
    sssu.steerpol( setup_.steerpol_ ).selectcomp( docomps );
    ctio.setObj( key() );

    uiSeisSelDlg dlg( this, ctio, sssu );
    if ( docomps && components().first() >= 0 )
	dlg.setCompNr( components().first() );

    if ( dlg.go() && dlg.ioObj() )
    {
	comp_idx_set comps;
	if ( docomps )
	    comps.add( dlg.compNr() );
	set( dlg.ioObj()->key(), &comps );
    }
}


void uiSeisProvider::selChgCB( CallBacker* )
{
    updCompsUi();
    if ( seldatafld_ )
	seldatafld_->setInput( key() );
    selectionChanged.trigger();
}


static uiObject* getBoxIOObj( uiComboBox* cb, uiListBox* lb )
{
    return lb ? &lb->asUiObject() : (uiObject*)cb;
}


void uiSeisProvider::attachFlds()
{
    auto* itmfld = getBoxIOObj( itmbox_, itmlist_ );
    maingrp_->setHAlignObj( itmfld );
    setHAlignObj( maingrp_ );

    if ( optcheck_ )
	maingrp_->attach( rightOf, optcheck_ );

    auto* leftfld = optcheck_ ? (uiObject*)optcheck_ : (uiObject*)lbl_;
    if ( geomtypebox_ )
    {
	if ( lbl_ )
	    lbl_->attach( rightOf, geomtypebox_ );
	else if ( optcheck_ )
	{
	    geomtypebox_->attach( rightOf, optcheck_ );
	    leftfld = geomtypebox_;
	}
    }

    if ( leftfld )
	itmfld->attach( rightOf, leftfld );

    auto* rightfld = itmfld;
    if ( selbut_ )
    {
	selbut_->attach( rightOf, itmfld );
	rightfld = selbut_;
    }
    if ( compbox_ || complist_ )
    {
	auto* compfld = getBoxIOObj( compbox_, complist_ );
	compfld->attach( rightOf, rightfld );
	rightfld = compfld;
    }

    seldatafld_->attach( alignedBelow, itmfld );
}


uiSeisProvider::idx_type uiSeisProvider::curGTIdx() const
{
    return geomtypebox_ ? geomtypebox_->currentItem() : 0;
}


uiSeisProvider::idx_type uiSeisProvider::curObjIdx() const
{
    return itmlist_ ? itmlist_->currentItem() : itmbox_->currentItem();
}


Seis::ProviderInfo* uiSeisProvider::curInfo() const
{
    const auto curobjidx = curObjIdx();
    if ( curobjidx < 0 )
       return nullptr;
    return mNonConst( data_.get(curGTIdx())->get( curobjidx ) );
}


Seis::GeomType uiSeisProvider::geomType() const
{
    return setup_.gts_[curGTIdx()];
}


void uiSeisProvider::fillUi()
{
    const auto& infolist = *data_.get( curGTIdx() );
    BufferStringSet nms;
    const idx_type defitm = infolist.addDispNames( nms );
    if ( itmbox_ )
    {
	const BufferString prevsel( itmbox_->text() );
	itmbox_->setEmpty();
	itmbox_->addItems( nms );
	if ( !prevsel.isEmpty() )
	    itmbox_->setText( prevsel );
	else if ( defitm >= 0 )
	    itmbox_->setCurrentItem( defitm );
    }
    else if ( itmlist_ )
    {
	const BufferString prevsel( itmlist_->getText() );
	itmlist_->setEmpty();
	itmlist_->addItems( nms );
	if ( !prevsel.isEmpty() )
	    itmlist_->setCurrentItem( prevsel );
	else if ( defitm >= 0 )
	    itmlist_->setCurrentItem( defitm );
    }
}


void uiSeisProvider::updUi()
{
    maingrp_->setSensitive( isActive() );
    if ( isActive() )
	updCompsUi();
}


void uiSeisProvider::updCompsUi()
{
    if ( !compbox_ && !complist_ )
	return;
    const auto* info = curInfo();
    if ( !info )
	return;

    const auto compnms = info->componentNames();
    auto* compfld = getBoxIOObj( compbox_, complist_ );
    compfld->setEmpty();
    if ( compbox_ )
	compbox_->addItems( compnms );
    else
	complist_->addItems( compnms );
    compfld->display( compnms.size() > 1 );
}


void uiSeisProvider::set( const DBKey& dbky, const comp_idx_set* comps )
{
    usrsetkey_ = dbky.isValid() ? toString( dbky ) : "";
    if ( usrsetkey_.isEmpty() )
	return;

    if ( seldatafld_ )
	seldatafld_->setInput( dbky );

    for ( auto igt=0; igt<data_.size(); igt++ )
    {
	auto& infolist = *data_.get( igt );
	for ( auto iinf=0; iinf<infolist.size(); iinf++ )
	{
	    auto& provinfo = *infolist.get( iinf );
	    if ( usrsetkey_ == provinfo.key() )
	    {
		setCurrent( igt, iinf );
		if ( comps )
		    setComponents( *comps );
		return;
	    }
	}
    }
}


void uiSeisProvider::setCurrent( idx_type ilist, idx_type iobj )
{
    if ( geomtypebox_ )
    {
	if ( geomtypebox_->currentItem() != ilist )
	{
	    geomtypebox_->setCurrentItem( ilist );
	    gtChgCB( 0 );
	}
    }

    if ( itmbox_ )
	itmbox_->setCurrentItem( iobj );
    else if ( itmlist_ )
	itmlist_->setCurrentItem( iobj );

    updUi();
}


void uiSeisProvider::setComponents( const comp_idx_set& comps )
{
    if ( comps.isEmpty() || comps.first() < 0 )
	return;

    if ( compbox_ )
	compbox_->setCurrentItem( comps.first() );
    else if ( complist_ )
	complist_->setChosen( comps );
}


void uiSeisProvider::setSelData( const SelData* sd )
{
    if ( seldatafld_ )
	seldatafld_->set( sd );
    else
    {
	delete fixedseldata_;
	fixedseldata_ = sd ? sd->clone() : 0;
    }
}


bool uiSeisProvider::isOK( bool showerr ) const
{
    const DBKey dbky( key() );
    if ( !dbky.isValid() )
    {
	if ( showerr )
	    uiMSG().error( tr("Not OK, TODO") );
	return false;
    }
    return true;
}


Seis::Provider* uiSeisProvider::get( bool showerr ) const
{
    Provider* ret = Provider::create( geomType() );
    auto uirv = ret->setInput( key() );
    if ( !uirv.isOK() )
	{ delete ret; if ( showerr ) uiMSG().error( uirv ); return nullptr; }

    ret->selectComponents( components() );
    ret->setSelData( getSelData() );
    return ret;
}


DBKey uiSeisProvider::key() const
{
    const auto curidx = curObjIdx();
    if ( curidx < 0 )
	return DBKey();

    const auto& infolist = *data_.get( curGTIdx() );
    return DBKey( infolist.get(curidx)->key() );
}


uiSeisProvider::comp_idx_set uiSeisProvider::components() const
{
    const auto* info = curInfo();
    if ( !info )
	return comp_idx_set( 1, -1 );

    const auto compnms = info->componentNames();
    const auto nrcomps = compnms.size();
    if ( nrcomps < 1 )
	return comp_idx_set( 1, -1 );
    else if ( nrcomps == 1 )
	return comp_idx_set( 1, 0 );

    comp_idx_set ret;
    if ( setup_.compselpol_ == AllComps )
	for ( auto idx=0; idx<nrcomps; idx++ )
	    ret.add( idx );
    else if ( compbox_ )
	ret.add( compbox_->currentItem() );
    else if ( complist_ )
	complist_->getChosen( ret );

    return ret;
}


Seis::SelData* uiSeisProvider::getSelData() const
{
    if ( seldatafld_ )
	return seldatafld_->get();
    return fixedseldata_ ? fixedseldata_->clone() : nullptr;
}


void uiSeisProvider::fillPar( IOPar& iop ) const
{
    auto* prov = get( false );
    if ( !prov )
    {
	iop.removeWithKey( sKey::ID() );
	iop.removeWithKey( sKey::Name() );
    }
    else
    {
	prov->fillPar( iop );
	delete prov;
    }
}


void uiSeisProvider::usePar( const IOPar& iop )
{
    const auto dbky = Provider::dbKey( iop );
    if ( !dbky.isValid() )
	return;
    auto* prov = Provider::create( iop );
    if ( !prov )
	return;

    set( prov->dbKey(), &prov->selectedComponents() );
    setSelData( prov->selData() );
    delete prov;
}
