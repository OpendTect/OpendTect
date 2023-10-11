/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiemattrsel.h"

#include "attribdescset.h"
#include "attribdesc.h"
#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "ctxtioobj.h"
#include "datapack.h"
#include "emioobjinfo.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "linekey.h"
#include "nladesign.h"
#include "nlamodel.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seispreload.h"
#include "seistrctr.h"
#include "separstr.h"
#include "trckeyzsampling.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjinserter.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uilistboxfilter.h"
#include "uimsg.h"
#include "uistrings.h"
#include "od_helpids.h"


using namespace Attrib;

static int sStoredID()		{ return 0; }
static int sSteerID()		{ return 1; }
static int sAttribID()		{ return 2; }
static int sNLAID()		{ return 3; }
static int sZDomainID()		{ return 4; }
static int sHorizonDataID()	{ return 5; }

static int sLastSelType = sStoredID();


uiEMAttrSelDlg::uiEMAttrSelDlg( uiParent* p, const uiAttrSelData& atd,
				const MultiID& mid, const Setup& stp )
    : uiDialog(p,uiDialog::Setup(uiStrings::sEmptyString(),mNoDlgTitle,
				 mODHelpKey(mAttrSelDlgNo_NNHelpID)))
    , attrdata_(atd)
    , usedasinput_(stp.isinp4otherattrib_)
    , showsteerdata_(stp.showsteeringdata_)
    , hormid_(mid)
{
    initAndBuild( toUiString(stp.seltxt_), stp.ignoreid_ );
}


void uiEMAttrSelDlg::initAndBuild( const uiString& seltxt,
				 Attrib::DescID ignoreid )
{
    //TODO: steering will never be displayed: on purpose?
    attrinf_ = new SelInfo( &attrdata_.attrSet(), attrdata_.nlamodel_,
			    is2D(), ignoreid );

    setCaption( uiStrings::sSelect() );

    uiString title = uiStrings::sSelect().append( seltxt );
    setTitleText( title );
    setName( title.getFullString() );

    createSelectionButtons();
    createSelectionFields();

    CtxtIOObj* ctio = mMkCtxtIOObj( SeisTrc );
    if ( ctio )
    {
	uiButtonGroup* butgrp = new uiButtonGroup( this, "Inserters selection",
						   OD::Vertical );
	const BufferStringSet nms;
	uiIOObjInserter::addInsertersToDlg( butgrp, *ctio, inserters_,
					    extselbuts_, nms );
	for ( auto* inserter : inserters_ )
	    mAttachCB( inserter->objInserterd, uiEMAttrSelDlg::objInserted );

	butgrp->attach( ensureBelow, selgrp_ );
    }

    int seltyp = sLastSelType;
    int storcur = -1, attrcur = -1, nlacur = -1;
    if ( attrdata_.nlamodel_ && attrdata_.outputnr_ >= 0 )
    {
	seltyp = sNLAID();
	nlacur = attrdata_.outputnr_;
    }
    else
    {
	const Desc* desc = attrdata_.attribid_.isValid()
			? attrdata_.attrSet().getDesc( attrdata_.attribid_ ) :0;
	if ( desc )
	{
	    seltyp = desc->isStored() ? 0 : 2;
	    if ( seltyp == sAttribID() )
		attrcur = attrinf_->attrnms_.indexOf( desc->userRef() );
	    else if ( storoutfld_ )
	    {
		LineKey lk( desc->userRef() );
		storcur = attrinf_->ioobjnms_.indexOf( lk.lineName() );
		//2D attrib is set in cubeSel, called from doFinalize
	    }
	}
	else
	{
	    // Defaults are the last ones added to attrib set
	    for ( int idx=attrdata_.attrSet().size()-1; idx!=-1; idx-- )
	    {
		const DescID attrid = attrdata_.attrSet().getID( idx );
		const Desc& ad = *attrdata_.attrSet().getDesc( attrid );
		if ( ad.isStored() && storcur == -1 )
		    storcur = attrinf_->ioobjnms_.indexOf( ad.userRef() );
		else if ( !ad.isStored() && attrcur == -1 )
		{
		    attrcur = attrinf_->attrnms_.indexOf( ad.userRef() );
		    seltyp = sAttribID();
		}
		if ( storcur != -1 && attrcur != -1 ) break;
	    }
	}

	if ( attrdata_.zdomaininfo_ && !zdomoutfld_->isEmpty() )
	{
	    seltyp = sZDomainID();
	    storcur = desc ? zdomoutfld_->indexOf(desc->userRef()) : -1;
	}
    }

    const bool havenlaouts = attrinf_->nlaoutnms_.size();
    if ( storcur == -1 )
	storcur = 0;
    if ( attrcur == -1 )
	attrcur = attrinf_->attrnms_.size()-1;
    if ( nlacur == -1 && havenlaouts )
	nlacur = 0;

    if ( storoutfld_  )
	storoutfld_->setCurrentItem( storcur );
    if ( attroutfld_ && attrcur != -1 )
	attroutfld_->setCurrentItem( attrcur );
    if ( nlaoutfld_ && nlacur != -1 )
	nlaoutfld_->setCurrentItem( nlacur );
    if ( zdomainbut_ )
	zdomoutfld_->setCurrentItem( storcur );

    if ( seltyp==sStoredID() )
	storbut_->setChecked( true );
    else if ( steerbut_ && seltyp==sSteerID() )
	steerbut_->setChecked( true );
    else if ( seltyp==sAttribID() )
	attrbut_->setChecked( true );
    else if ( nlabut_ && seltyp==sNLAID() )
	nlabut_->setChecked( true );
    else if ( zdomainbut_ && seltyp==sZDomainID() )
	zdomainbut_->setChecked( true );
    else if ( seltyp==sHorizonDataID() )
	hordatabut_->setChecked( true );

    mAttachCB( preFinalize(), uiEMAttrSelDlg::doFinalize );
}


uiEMAttrSelDlg::~uiEMAttrSelDlg()
{
    detachAllNotifiers();

    delete attribfilterfld_;
    delete selgrp_;
    delete attrinf_;
    deepErase( inserters_ );
}


void uiEMAttrSelDlg::doFinalize( CallBacker* )
{
    selDone(0);
    in_action_ = true;
}


void uiEMAttrSelDlg::createSelectionButtons()
{
    const bool havenlaouts = attrinf_->nlaoutnms_.size();
    const bool haveattribs = attrinf_->attrnms_.size();
    const bool havestored = attrinf_->ioobjnms_.size();
    const bool havesteered = attrinf_->steernms_.size();

    selgrp_ = new uiButtonGroup( this, "Input selection", OD::Vertical );
    storbut_ = new uiRadioButton( selgrp_, uiStrings::sStored() );
    mAttachCB( storbut_->activated, uiEMAttrSelDlg::selDone );
    storbut_->setSensitive( havestored );

    if ( showsteerdata_ )
    {
	steerbut_ = new uiRadioButton( selgrp_, uiStrings::sSteering() );
	mAttachCB( steerbut_->activated, uiEMAttrSelDlg::selDone );
	steerbut_->setSensitive( havesteered );
    }

    attrbut_ = new uiRadioButton( selgrp_, uiStrings::sAttribute(mPlural) );
    attrbut_->setSensitive( haveattribs );
    mAttachCB( attrbut_->activated, uiEMAttrSelDlg::selDone );

    if ( havenlaouts )
    {
	nlabut_ = new uiRadioButton( selgrp_,
			      toUiString(attrdata_.nlamodel_->nlaType(false)) );
	nlabut_->setSensitive( havenlaouts );
	mAttachCB( nlabut_->activated, uiEMAttrSelDlg::selDone );
    }

    if ( attrdata_.zdomaininfo_ )
    {
	BufferStringSet nms;
	SelInfo::getZDomainItems( *attrdata_.zdomaininfo_, is2D(), nms );
	zdomainbut_ = new uiRadioButton( selgrp_,
				   toUiString(attrdata_.zdomaininfo_->key()) );
	zdomainbut_->setSensitive( !nms.isEmpty() );
	mAttachCB( zdomainbut_->activated, uiEMAttrSelDlg::selDone );
    }

    hordatabut_ = new uiRadioButton( selgrp_, uiStrings::sHorizonData() );
    mAttachCB( hordatabut_->activated, uiEMAttrSelDlg::selDone );
}


template <class T>
static void setPreloadIcon( uiListBox* lb, const T& ids )
{
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const MultiID mid( ids.get(idx) );
	const char* iconnm =
		Seis::PLDM().isPresent(mid) ? "preloaded" : "empty";
	lb->setIcon( idx, iconnm );
    }
}


void uiEMAttrSelDlg::createSelectionFields()
{
    const bool havenlaouts = attrinf_->nlaoutnms_.size();
    const bool haveattribs = attrinf_->attrnms_.size();

    storoutfld_ = new uiListBox( this, "Stored output" );
    storoutfld_->addItems( attrinf_->ioobjnms_ );
    storoutfld_->setHSzPol( uiObject::Wide );
    mAttachCB( storoutfld_->selectionChanged, uiEMAttrSelDlg::cubeSel );
    mAttachCB( storoutfld_->doubleClicked, uiEMAttrSelDlg::accept );
    storoutfld_->attach( rightOf, selgrp_ );
    setPreloadIcon( storoutfld_, attrinf_->ioobjids_ );
    storoutfld_->resizeWidthToContents();

    steeroutfld_ = new uiListBox( this, "Steered output" );
    steeroutfld_->addItems( attrinf_->steernms_ );
    mAttachCB( steeroutfld_->selectionChanged, uiEMAttrSelDlg::cubeSel );
    mAttachCB( steeroutfld_->doubleClicked, uiEMAttrSelDlg::accept );
    steeroutfld_->attach( rightOf, selgrp_ );
    setPreloadIcon( steeroutfld_, attrinf_->steerids_ );

    filtfld_ = new uiGenInput( this, uiStrings::sFilter() );
    filtfld_->attach( centeredAbove, storoutfld_ );
    mAttachCB( filtfld_->valuechanged, uiEMAttrSelDlg::filtChg );

    compfld_ = new uiLabeledComboBox( this, tr("Component"), "Compfld" );
    compfld_->box()->setStretch( 2, 0 );
    compfld_->attach( alignedBelow, storoutfld_ );
    compfld_->attach( ensureBelow, steeroutfld_ );

    if ( haveattribs )
    {
	attroutfld_ = new uiListBox( this, "Output attributes" );
	attroutfld_->addItems( attrinf_->attrnms_ );
	attroutfld_->setHSzPol( uiObject::Wide );
	mAttachCB( attroutfld_->doubleClicked, uiEMAttrSelDlg::accept );
	attroutfld_->attach( ensureRightOf, selgrp_ );

	attribfilterfld_ = new uiListBoxFilter( *attroutfld_ );
	attribfilterfld_->setItems( attrinf_->attrnms_ );
    }

    if ( havenlaouts )
    {
	nlaoutfld_ = new uiListBox( this, "Output NLAs" );
	nlaoutfld_->addItems( attrinf_->nlaoutnms_ );
	nlaoutfld_->setHSzPol( uiObject::Wide );
	mAttachCB( nlaoutfld_->doubleClicked, uiEMAttrSelDlg::accept );
	nlaoutfld_->attach( rightOf, selgrp_ );
    }

    if ( attrdata_.zdomaininfo_ )
    {
	BufferStringSet nms;
	SelInfo::getZDomainItems( *attrdata_.zdomaininfo_, is2D(), nms );
	zdomoutfld_ = new uiListBox( this, "ZDomain output" );
	zdomoutfld_->addItems( nms );
	zdomoutfld_->setHSzPol( uiObject::Wide );
	mAttachCB( zdomoutfld_->selectionChanged, uiEMAttrSelDlg::cubeSel );
	mAttachCB( zdomoutfld_->doubleClicked, uiEMAttrSelDlg::accept );
	zdomoutfld_->attach( rightOf, selgrp_ );
	zdomoutfld_->attach( heightSameAs, storoutfld_ );
    }

    const EM::IOObjInfo info( hormid_ );
    BufferStringSet datanames;
    info.getAttribNames( datanames );
    hordatafld_ = new uiListBox( this, "Horizon Data" );
    hordatafld_->addItems( datanames );
    hordatafld_->setHSzPol( uiObject::Wide );
    hordatafld_->attach( ensureRightOf, selgrp_ );
    mAttachCB( hordatafld_->doubleClicked, uiEMAttrSelDlg::accept );
}


int uiEMAttrSelDlg::selType() const
{
    if ( steerbut_ && steerbut_->isChecked() )
	return sSteerID();
    if ( attrbut_->isChecked() )
	return sAttribID();
    if ( nlabut_ && nlabut_->isChecked() )
	return sNLAID();
    if ( zdomainbut_ && zdomainbut_->isChecked() )
	return sZDomainID();
    if ( hordatabut_ && hordatabut_->isChecked() )
	return sHorizonDataID();

    return sStoredID();
}


void uiEMAttrSelDlg::selDone( CallBacker* )
{
    if ( !selgrp_ )
	return;

    const int seltyp = selType();
    if ( attroutfld_ )
	attroutfld_->display( seltyp==sAttribID() );
    if ( nlaoutfld_ )
	nlaoutfld_->display( seltyp==sNLAID() );
    if ( zdomoutfld_ )
	zdomoutfld_->display( seltyp==sZDomainID() );
    if ( storoutfld_ || steeroutfld_ )
    {
	if ( storoutfld_ )
	    storoutfld_->display( seltyp==sStoredID() );
	if ( steeroutfld_ )
	    steeroutfld_->display( seltyp==sSteerID() );
    }
    if ( hordatafld_ )
	hordatafld_->display( seltyp==sHorizonDataID() );

    const bool isstoreddata = seltyp==sStoredID() || seltyp==sSteerID();
    const bool issteerdata = seltyp==sSteerID();
    filtfld_->display( isstoreddata );
    compfld_->display( issteerdata );

    cubeSel(0);
}


void uiEMAttrSelDlg::filtChg( CallBacker* cb )
{
    if ( !storoutfld_ || !filtfld_ )
	return;

    const bool issteersel = selType() == sSteerID();
    uiListBox* outfld = issteersel ? steeroutfld_ : storoutfld_;
    BufferStringSet& nms = issteersel ? attrinf_->steernms_
				      : attrinf_->ioobjnms_;
    outfld->setEmpty();
    attrinf_->fillStored( issteersel, filtfld_->text() );
    if ( nms.isEmpty() ) return;

    outfld->addItems( nms );
    outfld->setCurrentItem( 0 );
    cubeSel( cb );
}


void uiEMAttrSelDlg::cubeSel( CallBacker* )
{
    if ( !storoutfld_ )
	return;

    const int seltyp = selType();
    if ( seltyp==sAttribID() || seltyp==sNLAID() || seltyp>sZDomainID() )
	return;

    BufferString ioobjkey;
    if ( seltyp==sStoredID() )
    {
	const int curitem = storoutfld_->currentItem();
	if ( attrinf_->ioobjids_.validIdx(curitem) )
	    ioobjkey = attrinf_->ioobjids_.get( curitem );
    }
    else if ( seltyp==sSteerID() )
    {
	const int curitem = steeroutfld_->currentItem();
	if ( attrinf_->steerids_.validIdx(curitem) )
	    ioobjkey = attrinf_->steerids_.get( curitem );
    }
    else if ( seltyp==sZDomainID() )
    {
	const int selidx = zdomoutfld_->currentItem();
	BufferStringSet nms;
	SelInfo::getZDomainItems( *attrdata_.zdomaininfo_, is2D(), nms );
	if ( nms.validIdx(selidx) )
	{
	    IOM().to( IOObjContext::Seis );
	    ConstPtrMan<IOObj> ioobj = IOM().getLocal( nms.get(selidx), 0 );
	    if ( ioobj )
		ioobjkey = ioobj->key();
	}
    }

    const bool isstoreddata = seltyp==sStoredID() || seltyp==sSteerID();
    filtfld_->display( isstoreddata );

    compfld_->box()->setCurrentItem(0);
    const MultiID key( ioobjkey.buf() );
    BufferStringSet compnms;
    SeisIOObjInfo::getCompNames( key, compnms );

    compfld_->box()->setEmpty();
    compfld_->box()->addItem( uiStrings::sAll() );
    compfld_->box()->addItems( compnms );
    compfld_->display( compnms.size()>=2 );
    if ( !in_action_ && compnms.validIdx(attrdata_.compnr_) )
	compfld_->box()->setCurrentItem( attrdata_.compnr_ + 1 );
}


bool uiEMAttrSelDlg::getAttrData( bool needattrmatch )
{
    DescSet* descset = nullptr;
    attrdata_.attribid_ = SelSpec::cAttribNotSel();
    attrdata_.outputnr_ = -1;

    if ( !insertedobjmid_.isUdf() )
    {
	PtrMan<IOObj> ioobj = IOM().get( insertedobjmid_ );
	if ( !ioobj )
	    return false;

	descset = usedasinput_
		? const_cast<DescSet*>( &attrdata_.attrSet() )
		: eDSHolder().getDescSet( is2D(), true );
	attrdata_.attribid_ = descset->getStoredID( ioobj->key(), 0, true );
	attrdata_.compnr_ = 0;
	if ( !usedasinput_ && descset )
	    attrdata_.setAttrSet( descset );

	return true;
    }

    if ( !selgrp_ || !in_action_ )
	return true;

    int selidx = -1;
    const int seltyp = selType();
    if ( seltyp==sStoredID() )
	selidx = storoutfld_->currentItem();
    else if ( seltyp==sSteerID() )
	selidx = steeroutfld_->currentItem();
    else if ( seltyp==sAttribID() )
	selidx = attribfilterfld_->getCurrent();
    else if ( seltyp==sNLAID() )
	selidx = nlaoutfld_->currentItem();
    else if ( seltyp==sZDomainID() )
	selidx = zdomoutfld_->currentItem();
    else if ( seltyp==sHorizonDataID() )
	selidx = hordatafld_->currentItem();
    if ( selidx < 0 )
	return false;

    if ( seltyp == sAttribID() )
	attrdata_.attribid_ = attrinf_->attrids_[selidx];
    else if ( seltyp == sNLAID() )
    {
	attrdata_.attribid_ = SelSpec::cOtherAttrib();
	attrdata_.outputnr_ = selidx;
    }
    else if ( seltyp == sZDomainID() )
    {
	if ( !attrdata_.zdomaininfo_ )
	    { pErrMsg( "Huh" ); return false; }

	BufferStringSet nms;
	SelInfo::getZDomainItems( *attrdata_.zdomaininfo_, is2D(), nms );
	IOM().to( IOObjContext::Seis );
	ConstPtrMan<IOObj> ioobj = IOM().getLocal( nms.get(selidx), 0 );
	if ( !ioobj )
	    return false;

	descset = usedasinput_
		? const_cast<DescSet*>( &attrdata_.attrSet() )
		: eDSHolder().getDescSet( is2D(), true );
	attrdata_.attribid_ = descset->getStoredID( ioobj->key(), 0, true );
	attrdata_.compnr_ = 0;
    }
    else if ( seltyp == sHorizonDataID() )
    {
	attrdata_.attribid_ = SelSpec::cOtherAttrib();
	attrdata_.attribid_.setStored( true );
	attrdata_.compnr_ = 0;
    }
    else
    {
	const int curselitmidx = compfld_->box()->currentItem();
	const bool canuseallcomps =
	    BufferString(compfld_->box()->textOfItem(curselitmidx))
			== BufferString(uiStrings::sAll().getFullString())
			&& compfld_->mainObject()->visible();
	attrdata_.compnr_ = canuseallcomps ? -1 : curselitmidx-1;
	if ( attrdata_.compnr_< 0 && !canuseallcomps )
	    attrdata_.compnr_ = 0;
	const MultiID& ioobjkey =
		seltyp==sStoredID() ? attrinf_->ioobjids_.get( selidx )
				    : attrinf_->steerids_.get( selidx );
	descset = usedasinput_
		? const_cast<DescSet*>( &attrdata_.attrSet() )
		: eDSHolder().getDescSet( is2D(), true );
	attrdata_.attribid_ = canuseallcomps && attrdata_.compnr_==-1
	    ? descset->getStoredID(ioobjkey, attrdata_.compnr_, true,true,"ALL")
	    : descset->getStoredID( ioobjkey, attrdata_.compnr_, true );
	if ( needattrmatch && !attrdata_.attribid_.isValid() )
	{
	    uiString msg = tr("Could not find the seismic data %1")
			 .arg(attrdata_.attribid_ == DescID::undef()
			 ? tr("in object manager")
			 : tr("on disk"));
	    uiMSG().error( msg );
	    return false;
	}
    }

    if ( !usedasinput_ && descset )
	attrdata_.setAttrSet( descset );

    return true;
}


const char* uiEMAttrSelDlg::zDomainKey() const
{ return attrdata_.zdomaininfo_ ? attrdata_.zdomaininfo_->key() : ""; }


bool uiEMAttrSelDlg::acceptOK( CallBacker* )
{
    sLastSelType = selType();
    return getAttrData(true);
}


void uiEMAttrSelDlg::objInserted( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const MultiID&, ky, cb );
    if ( !ky.isUdf() )
    {
	insertedobjmid_ = ky;
	accept( 0 );
    }
}


void uiEMAttrSelDlg::fillSelSpec( SelSpec& as ) const
{
    attrdata_.fillSelSpec( as );
    if ( selType() == sHorizonDataID() )
	as.setUserRef( hordatafld_->getText() );
}


BufferString uiEMAttrSelDlg::getHorizonDataName() const
{
    return selType() == sHorizonDataID() ? hordatafld_->getText() : "";
}



// uiEMAttrSel
uiString uiEMAttrSel::cDefLabel() { return uiStrings::sInputData(); }

uiEMAttrSel::uiEMAttrSel( uiParent* p, const MultiID& hormid,
			  const DescSet& ads, const uiString& lbl,
			  DescID curid, bool isinp4otherattrib )
    : uiIOSelect(p,uiIOSelect::Setup(lbl.isSet()?lbl:cDefLabel()),
		 mCB(this,uiEMAttrSel,doSel))
    , attrdata_(ads,false)
    , usedasinput_(isinp4otherattrib)
    , hormid_(hormid)
{
    attrdata_.attribid_ = curid;
    updateInput();
    inp_->setEditable( true );
    inp_->setReadOnly( true );
}


uiEMAttrSel::~uiEMAttrSel()
{}


void uiEMAttrSel::setDescSet( const DescSet* ads )
{
    attrdata_.setAttrSet( ads );
    updateInput();
}


void uiEMAttrSel::setNLAModel( const NLAModel* mdl )
{
    attrdata_.nlamodel_ = mdl;
}


void uiEMAttrSel::setSelSpec( const Attrib::SelSpec* selspec )
{
    if ( !selspec ) return;

    attrdata_.attribid_ = selspec->id();
    updateInput();
}


void uiEMAttrSel::setIgnoreDesc( const Desc* ad )
{
    ignoreid_ = ad ? ad->id() : DescID::undef();
}


void uiEMAttrSel::updateInput()
{
    SeparString bs( 0, ':' );
    bs += attrdata_.attribid_.asInt();
    bs += getYesNoString( attrdata_.attribid_.isStored() );
    bs += attrdata_.outputnr_;
    if ( attrdata_.compnr_ > -1 )
	bs += attrdata_.compnr_;

    setInput( bs );
}


const char* uiEMAttrSel::userNameFromKey( const char* txt ) const
{
    const StringView txtstr = txt;
    if ( txtstr.isEmpty() )
	return "";

    if ( !horizondataname_.isEmpty() )
	return horizondataname_.buf();

    SeparString bs( txt, ':' );
    if ( bs.size() == 1 )
    {
	const MultiID dbky( txt );
	if ( !dbky.isUdf() && dbky.isInMemoryID() )
	    return DataPackMgr::nameOf( dbky );

	return "";
    }

    if ( bs.size() < 3 )
	return "";

    const int id = toInt( bs[0] );
    const bool isstored = toBool( bs[1], true );
    const DescID attrid( id, isstored );
    const int outnr = toInt( bs[2] );

    if ( !attrid.isValid() )
    {
	if ( !attrdata_.nlamodel_ || outnr < 0 )
	    return "";
	if ( outnr >= attrdata_.nlamodel_->design().outputs_.size() ||
		!attrdata_.nlamodel_->design().outputs_[outnr] )
	    return "<error>";

	const char* nm = attrdata_.nlamodel_->design().outputs_[outnr]->buf();
	return IOObj::isKey(nm) ? IOM().nameOf(nm) : nm;
    }

    const DescSet& descset = attrid.isStored() ?
	*eDSHolder().getDescSet( is2D(), true ) : attrdata_.attrSet();
    const Desc* ad = descset.getDesc( attrid );
    usrnm_ = ad ? ad->userRef() : "";
    return usrnm_.buf();
}


void uiEMAttrSel::getHistory( const IOPar& iopar )
{
    uiIOSelect::getHistory( iopar );
    updateInput();
}


bool uiEMAttrSel::getRanges( TrcKeyZSampling& cs ) const
{
    if ( !attrdata_.attribid_.isValid() )
	return false;

    const Desc* desc = attrdata_.attrSet().getDesc( attrdata_.attribid_ );
    if ( !desc ) return false;

    const MultiID mid( desc->getStoredID(true).buf() );
    return SeisTrcTranslator::getRanges( mid, cs,
					 desc->is2D() ? getInput() : 0 );
}


void uiEMAttrSel::doSel( CallBacker* )
{
    uiEMAttrSelDlg::Setup setup( lbl_ ? lbl_->text() : cDefLabel() );
    setup.ignoreid(ignoreid_).isinp4otherattrib(usedasinput_)
	 .showsteeringdata(showsteeringdata_);
    uiEMAttrSelDlg dlg( this, attrdata_, hormid_, setup );
    if ( !dlg.go() )
    {
	attrdata_.attribid_ = SelSpec::cNoAttrib();
	return;
    }

    attrdata_.attribid_ = dlg.attribID();
    attrdata_.outputnr_ = dlg.outputNr();
    attrdata_.compnr_ = dlg.compNr();
    if ( !usedasinput_ )
	attrdata_.setAttrSet( &dlg.getAttrSet() );

    seltype_ = dlg.selType();
    horizondataname_ = dlg.getHorizonDataName();
    updateInput();
    selok_ = true;
}


void uiEMAttrSel::processInput()
{
    BufferString inp = getInput();
    const DescSet& descset = usedasinput_ ? attrdata_.attrSet()
				: *eDSHolder().getDescSet( is2D(), true );
    bool useseltyp = seltype_ >= sStoredID();
    if ( !useseltyp )
    {
	const Desc* adesc = descset.getDesc( attrdata_.attribid_ );
	if ( adesc )
	{
	    useseltyp = true;
	    seltype_ = adesc->isStored() ? 0 : 1;
	}
    }

    attrdata_.attribid_ = descset.getID( inp, true, !seltype_, useseltyp );
    if ( !attrdata_.attribid_.isValid() && !usedasinput_ )
	attrdata_.attribid_ = attrdata_.attrSet().getID( inp, true );
    attrdata_.outputnr_ = -1;
    if ( !attrdata_.attribid_.isValid() && attrdata_.nlamodel_ )
    {
	const BufferStringSet& outnms( attrdata_.nlamodel_->design().outputs_ );
	const BufferString nodenm = IOObj::isKey( inp ) ?
				IOM().nameOf( inp.buf() ) : inp.buf();
	for ( int idx=0; idx<outnms.size(); idx++ )
	{
	    const BufferString& outstr = *outnms[idx];
	    const char* desnm = IOObj::isKey( outstr ) ?
				IOM().nameOf( outstr.buf() ) : outstr.buf();
	    if ( nodenm == desnm )
		{ attrdata_.outputnr_ = idx; break; }
	}
    }

    updateInput();
}


void uiEMAttrSel::fillSelSpec( SelSpec& as ) const
{
    attrdata_.fillSelSpec( as );
    if ( !horizondataname_.isEmpty() )
	as.setUserRef( horizondataname_.buf() );
}


const char* uiEMAttrSel::getAttrName() const
{
    mDeclStaticString( ret );

    ret = getInput();
    return ret.buf();
}


bool uiEMAttrSel::checkOutput( const IOObj& ioobj ) const
{
    if ( !attrdata_.attribid_.isValid() && attrdata_.outputnr_ < 0 )
    {
	uiMSG().error( tr("Please select the input") );
	return false;
    }

    if ( is2D() && !SeisTrcTranslator::is2D(ioobj) )
    {
	uiMSG().error( tr("Can only store this in a 2D Data set") );
	return false;
    }

    //TODO check cyclic dependencies and bad stored IDs
    return true;
}


void uiEMAttrSel::setObjectName( const char* nm )
{
    inp_->setName( nm );
    if ( selbut_ )
    {
	const char* butnm = selbut_->name();
	selbut_->setName( BufferString(butnm," ",nm) );
    }
}



// uiEMRGBAttrSelDlg
uiEMRGBAttrSelDlg::uiEMRGBAttrSelDlg( uiParent* p, const MultiID& hormid,
				      const Attrib::DescSet& ds )
    : uiDialog(p,Setup(tr("Select RGB Attributes"),mNoDlgTitle,
			mODHelpKey(mRGBAttrSelHelpID)))
{
    rfld_ = new uiEMAttrSel( this, hormid, ds );
    rfld_->showSteeringData( true );
    rfld_->setLabelText( tr("Red Attribute") );

    gfld_ = new uiEMAttrSel( this, hormid, ds );
    gfld_->showSteeringData( true );
    gfld_->setLabelText( tr("Green Attribute") );
    gfld_->attach( alignedBelow, rfld_ );

    bfld_ = new uiEMAttrSel( this, hormid, ds );
    bfld_->showSteeringData( true );
    bfld_->setLabelText( tr("Blue Attribute") );
    bfld_->attach( alignedBelow, gfld_ );

    tfld_ = new uiEMAttrSel( this, hormid, ds );
    tfld_->showSteeringData( true );
    tfld_->setLabelText( tr("Alpha Attribute") );
    tfld_->attach( alignedBelow, bfld_ );
}


uiEMRGBAttrSelDlg::~uiEMRGBAttrSelDlg()
{
}


void uiEMRGBAttrSelDlg::setSelSpec( const TypeSet<Attrib::SelSpec>& as )
{
    if ( as.size() != 4 )
	return;

    rfld_->setSelSpec( &as[0] );
    gfld_->setSelSpec( &as[1] );
    bfld_->setSelSpec( &as[2] );
    tfld_->setSelSpec( &as[3] );
}


void uiEMRGBAttrSelDlg::fillSelSpec( TypeSet<Attrib::SelSpec>& as ) const
{
    if ( as.size() != 4 )
	as.setSize( 4 );

    rfld_->fillSelSpec( as[0] );
    gfld_->fillSelSpec( as[1] );
    bfld_->fillSelSpec( as[2] );
    tfld_->fillSelSpec( as[3] );
}


bool uiEMRGBAttrSelDlg::acceptOK( CallBacker* )
{
    return true;
}
