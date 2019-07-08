/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          May 2005
________________________________________________________________________

-*/

#include "uiattrsel.h"
#include "uiattrdesced.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribsel.h"
#include "attribfactory.h"
#include "hilbertattrib.h"

#include "ioobj.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seispreload.h"
#include "trckeyzsampling.h"
#include "zdomain.h"

#include "nlamodel.h"
#include "nladesign.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjinserter.h"
#include "uilistbox.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "od_helpids.h"

uiString uiAttrSel::sDefLabel()		{ return uiStrings::sInputData(); }
uiString uiAttrSel::sQuantityToOutput() { return tr("Quantity to output"); }


uiAttrSelData::uiAttrSelData( bool is2d )
    : attrset_(&Attrib::DescSet::global(is2d))
{
    init();
}

uiAttrSelData::uiAttrSelData( const DescSet& ads )
    : attrset_(&ads)
{
    init();
}


uiAttrSelData::~uiAttrSelData()
{
    detachAllNotifiers();
}


void uiAttrSelData::init()
{
    nlamodel_ = 0;
    zdomaininfo_ = 0;
    setUndef();
    if ( !attrset_ )
	{ pErrMsg("Global Attrsets not created"); }
    else
	mAttachCB( attrset_->aboutToBeDeleted, uiAttrSelData::descSetDel );
}


bool uiAttrSelData::isUndef() const
{
    return !(isNLA() || attribid_.isValid());
}


void uiAttrSelData::setUndef()
{
    attribid_.setInvalid();
    nr_ = -1;
}


bool uiAttrSelData::is2D() const
{
    return attrset_->is2D();
}

bool uiAttrSelData::isNLA() const
{
    return nlamodel_ && !attribid_.isValid() && nr_ >= 0;
}


void uiAttrSelData::descSetDel( CallBacker* )
{
    setAttrSet( Attrib::DescSet::global( is2D() ) );
}


void uiAttrSelData::setAttrSet( const DescSet& newds )
{
    if ( &newds == attrset_ )
	return;

    mDetachCB( attrset_->aboutToBeDeleted, uiAttrSelData::descSetDel );
    attrset_ = &newds;
    mAttachCB( attrset_->aboutToBeDeleted, uiAttrSelData::descSetDel );
}


void uiAttrSelData::fillSelSpec( SelSpec& selspec ) const
{
    if ( isNLA() )
    {
	selspec.set( 0, DescID(nr_), true, "" );
	selspec.setRefFromID( *nlamodel_ );
    }
    else
    {
	selspec.set( 0, attribid_, false, "" );
	selspec.setRefFromID( *attrset_ );
    }

    if ( is2D() )
	selspec.set2D();
}


uiAttrSelectionObj::uiAttrSelectionObj( const uiAttrSelData& attrsd,
					bool showsteer )
    : seldata_(attrsd)
    , attrinf_(0)
    , showsteerdata_(showsteer)
{
}


uiAttrSelectionObj::~uiAttrSelectionObj()
{
    delete attrinf_;
}


const char* uiAttrSelectionObj::zDomainKey() const
{
    return seldata_.zdomaininfo_ ? seldata_.zdomaininfo_->key() : "";
}


bool uiAttrSelectionObj::have( SelType seltyp ) const
{
    switch ( seltyp )
    {
	case Stored:	return !attrinf_->ioobjnms_.isEmpty();
	case Steer:	return !attrinf_->steernms_.isEmpty();
	case Attrib:	return !attrinf_->attrnms_.isEmpty();
	case NLA:	return !attrinf_->nlaoutnms_.isEmpty();
	default:	{ pErrMsg("New SelTyp"); return false; }
    }
}


uiString uiAttrSelectionObj::selTypeDispStr( SelType seltype ) const
{
    switch ( seltype )
    {
	case Stored:	return uiStrings::sStored();
	case Steer:	return uiStrings::sSteering();
	case Attrib:	return uiStrings::sAttribute();
	case NLA:
	{
	    if ( !seldata_.nlamodel_ )
		{ pErrMsg("Huh? NLA Model"); return uiString::empty(); }
	    return toUiString( seldata_.nlamodel_->nlaType(false) );
	}
	default:
	{
	    pErrMsg("New SelType");
	    return uiString::empty();
	}
    }
}


BufferString uiAttrSelectionObj::selTypeIconID( SelType seltype )
{
    BufferString ret( "attribtype_" );
    switch ( seltype )
    {
	case Steer:	ret.add( "steer" );	break;
	case Attrib:	ret.add( "attrib" );	break;
	case NLA:	ret.add( "nla" );	break;
	default:	{ pFreeFnErrMsg("New sel Type"); } // no break
	case Stored:	ret.add( "stored" );	break;
    }
    return ret;
}


void uiAttrSelectionObj::fillAttrInf()
{
    delete attrinf_;
    attrinf_ = new Attrib::SelInfo( seldata_.attrSet(), ignoreid_,
			    seldata_.nlamodel_, seldata_.zdomaininfo_ );
    if ( !dpids_.isEmpty() )
	attrinf_->fillSynthetic( false, dpids_ );
}


void uiAttrSelectionObj::fillSelSpec( SelSpec& as ) const
{
    seldata_.fillSelSpec( as );
}


#define muiAttrSelDlgConstrInitList \
	uiDialog(p,uiDialog::Setup(uiString::empty(),mNoDlgTitle, \
					mODHelpKey(mAttrSelDlgNo_NNHelpID))) \
	, uiAttrSelectionObj(atd,stp.showsteeringdata_) \
	, steertypsel_(0) \
	, steerentriesfld_(0) \
	, attribtypsel_(0) \
	, attribentriesfld_(0) \
	, nlatypsel_(0) \
	, nlaentriesfld_(0) \
	, fully_finalised_(false)


uiAttrSelDlg::uiAttrSelDlg( uiParent* p, const uiAttrSelData& atd,
			    const Setup& stp )
    : muiAttrSelDlgConstrInitList
{
    initAndBuild( stp );
}

uiAttrSelDlg::uiAttrSelDlg( uiParent* p, const uiAttrSelData& atd,
			    const DPIDSet& dpfids, const Setup& stp )
    : muiAttrSelDlgConstrInitList
{
    dpids_ = dpfids;
    initAndBuild( stp );
}


void uiAttrSelDlg::initAndBuild( const Setup& stp )
{
    showsteerdata_ = stp.showsteeringdata_;
    ignoreid_ = stp.ignoreid_;
    fillAttrInf();

    setCaption( uiStrings::sInputData() );
    uiString title = uiStrings::phrSelect( stp.seltxt_ );
    setTitleText( title );
    setName( toString(title) );

    uiGroup* typselgrp = createSelectionButtons();
    uiGroup* selfldgrp = createSelectionFields();
    selfldgrp->attach( rightOf, typselgrp );

    CtxtIOObj* ctio = mMkCtxtIOObj( SeisTrc );
    if ( ctio )
    {
	uiButtonGroup* butgrp = new uiButtonGroup( this, "Inserters selection",
						   OD::Vertical );
	uiIOObjInserter::addInsertersToDlg( butgrp, *ctio, inserters_,
					    insbuts_ );
	for ( int idx=0; idx<inserters_.size(); idx++ )
	{
	    inserters_[idx]->objectInserted.notify(
		    mCB(this,uiAttrSelDlg,objInsertedCB) );
	}
	butgrp->attach( rightAlignedBelow, typselgrp );
    }

    SelType seltyp = Stored;
    int storcur = -1, attrcur = -1, nlacur = -1;
    if ( seldata_.isNLA() )
    {
	seltyp = NLA;
	nlacur = seldata_.outputNr();
    }
    else
    {
	const Desc* desc = seldata_.attribid_.isValid()
			? seldata_.attrSet().getDesc( seldata_.attribid_ ) :0;
	if ( !desc )
	{
	    const DescID descid( seldata_.attrSet().getDefaultTargetID() );
	    if ( descid.isValid() )
		desc = seldata_.attrSet().getDesc( descid );
	}
	if ( desc )
	{
	    seltyp = desc->isStored() ? Stored : Attrib;
	    const BufferString usrref( desc->userRef() );
	    if ( seltyp == Attrib )
		attrcur = attrinf_->attrnms_.indexOf( usrref );
	    else if ( storedentriesfld_ )
		storcur = attrinf_->ioobjnms_.indexOf( usrref );
	}
    }

    if ( storcur < 0 )
	storcur = 0;
    if ( attrcur < 0 )
	attrcur = attrinf_->attrnms_.size()-1;
    if ( nlacur < 0 && have(NLA) )
	nlacur = 0;

    storedentriesfld_->setCurrentItem( storcur );
    if ( attrcur >= 0 )
	attribentriesfld_->setCurrentItem( attrcur );
    if ( nlaentriesfld_ && nlacur >= 0 )
	nlaentriesfld_->setCurrentItem( nlacur );

    if ( steertypsel_ && seltyp == Steer )
	steertypsel_->setChecked( true );
    else if ( attribtypsel_ && seltyp == Attrib )
	attribtypsel_->setChecked( true );
    else if ( nlatypsel_ && seltyp == NLA )
	nlatypsel_->setChecked( true );
    else
	storedtypsel_->setChecked( true );

    preFinalise().notify( mCB(this,uiAttrSelDlg,finaliseWinCB) );
}


uiAttrSelDlg::~uiAttrSelDlg()
{
    deepErase( inserters_ );
}


void uiAttrSelDlg::finaliseWinCB( CallBacker* )
{
    selDoneCB( 0 );
    fully_finalised_ = true;
}


uiGroup* uiAttrSelDlg::createSelectionButtons()
{
#define mCrSelBut(typ,enm) \
    { \
	typ##typsel_ = new uiRadioButton( bgrp, selTypeDispStr( enm ) ); \
	typ##typsel_->setIcon( selTypeIconID(enm) ); \
	typ##typsel_->setSensitive( have(enm) ); \
	typ##typsel_->activated.notify( seldonecb ); \
	typsels_ += typ##typsel_; \
    }

    const CallBack seldonecb( mCB(this,uiAttrSelDlg,selDoneCB) );
    uiButtonGroup* bgrp = new uiButtonGroup( this, "Input selection",
					     OD::Vertical );

    mCrSelBut( stored, Stored );
    if ( showsteerdata_ && have(Steer) )
	mCrSelBut( steer, Steer );
    if ( have(Attrib) )
	mCrSelBut( attrib, Attrib );
    if ( have(NLA) )
	mCrSelBut( nla, NLA );
    return bgrp;
}


uiGroup* uiAttrSelDlg::createSelectionFields()
{
    filtfld_ = new uiGenInput( this, uiStrings::sFilter(), "*" );
    filtfld_->valuechanged.notify( mCB(this,uiAttrSelDlg,filtChgCB) );

    uiGroup* fldgrp = new uiGroup( this, "Entry fields group" );
    const CallBack cubeselcb( mCB(this,uiAttrSelDlg,cubeSelCB) );

#define mCreateEntriesFld(fld,nm,memb) \
    fld = new uiListBox( fldgrp, nm ); \
    fld->addItems( attrinf_->memb ); \
    fld->setHSzPol( uiObject::Wide ); \
    fld->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) ); \
    entriesflds_ += fld

    mCreateEntriesFld( storedentriesfld_, "Stored data", ioobjnms_ );
    storedentriesfld_->selectionChanged.notify( cubeselcb );
    fldgrp->setHAlignObj( storedentriesfld_ );
    for ( int idx=0; idx<attrinf_->ioobjids_.size(); idx++ )
    {
	const DBKey dbky( attrinf_->ioobjids_.get(idx) );
	storedentriesfld_->setIcon( idx, Seis::PLDM().isPresent(dbky) ?
					 "preloaded" : "empty" );
    }

    if ( have(Steer) )
    {
	mCreateEntriesFld( steerentriesfld_, "Steered data", steernms_ );
	steerentriesfld_->selectionChanged.notify( cubeselcb );
    }

    if ( have(Attrib) )
    {
	mCreateEntriesFld( attribentriesfld_, "Attributes", attrnms_ );
	if ( have(NLA) )
	{
	    mCreateEntriesFld( nlaentriesfld_, "NLA outputs", nlaoutnms_ );
	}
    }
    fldgrp->attach( centeredBelow, filtfld_ );

    compfld_ = new uiLabeledComboBox( this, uiStrings::sComponent(), "Compfld");
    compfld_->attach( rightAlignedBelow, fldgrp );

    return fldgrp;
}


uiAttrSelectionObj::SelType uiAttrSelDlg::selType() const
{
    if ( steertypsel_ && steertypsel_->isChecked() )
	return Steer;
    else if ( attribtypsel_ && attribtypsel_->isChecked() )
	return Attrib;
    else if ( nlatypsel_ && nlatypsel_->isChecked() )
	return NLA;

    return Stored;
}


uiListBox* uiAttrSelDlg::entryList4Type( SelType seltyp )
{
    switch ( seltyp )
    {
	case Stored:	return storedentriesfld_;
	case Steer:	return steerentriesfld_;
	case Attrib:	return attribentriesfld_;
	case NLA:	return nlaentriesfld_;
	default:	{ pErrMsg("New seltype"); return 0; }
    }
}


void uiAttrSelDlg::selDoneCB( CallBacker* c )
{
    const SelType seltyp = selType();
    uiListBox* fld = entryList4Type( seltyp );
    if ( !fld )
	fld = storedentriesfld_;
    for ( int idx=0; idx<typsels_.size(); idx++ )
	entriesflds_[idx]->display( fld == entriesflds_[idx] );

    const bool isstoreddata = seltyp==Stored || seltyp==Steer;
    const bool issteerdata = seltyp==Steer;
    filtfld_->display( isstoreddata );
    compfld_->display( issteerdata );

    cubeSelCB(0);
}


void uiAttrSelDlg::filtChgCB( CallBacker* c )
{
    const bool issteersel = selType() == 1;
    uiListBox* outfld = issteersel ? steerentriesfld_ : storedentriesfld_;
    BufferStringSet& nms = issteersel ? attrinf_->steernms_
				      : attrinf_->ioobjnms_;
    outfld->setEmpty();
    attrinf_->fillStored( issteersel, filtfld_->text() );
    if ( nms.isEmpty() )
	return;

    outfld->addItems( nms );
    outfld->setCurrentItem( 0 );
    cubeSelCB( c );
}


void uiAttrSelDlg::cubeSelCB( CallBacker* c )
{
    const SelType seltyp = selType();
    if ( seltyp==Attrib || seltyp==NLA )
	return;

    DBKey ioobjkey;
    if ( seltyp==Stored )
    {
	const int curitem = storedentriesfld_->currentItem();
	if ( attrinf_->ioobjids_.validIdx(curitem) )
	    ioobjkey = attrinf_->ioobjids_.get( curitem );
    }
    else if ( seltyp==Steer )
    {
	const int curitem = steerentriesfld_->currentItem();
	if ( attrinf_->steerids_.validIdx(curitem) )
	    ioobjkey = attrinf_->steerids_.get( curitem );
    }

    const bool is2d = ioobjkey.isInvalid() ? false
					   : SeisIOObjInfo(ioobjkey).is2D();
    const bool isstoreddata = seltyp==Stored || seltyp==Steer;
    filtfld_->display( !is2d && isstoreddata );

    const SeisIOObjInfo ioobjinf( ioobjkey ); BufferStringSet compnms;
    ioobjinf.getComponentNames( compnms );

    compfld_->box()->setEmpty();
    compfld_->box()->addItem( uiStrings::sAll() );
    compfld_->box()->addItems( compnms );
    compfld_->display( compnms.size()>=2 );
    compfld_->box()->setCurrentItem( 0 );
}


bool uiAttrSelDlg::getAttrData( bool needattrmatch )
{
    const DescSet& descset = seldata_.attrSet();
    seldata_.setUndef();

    if ( insertedobjdbky_.isValid() )
    {
	PtrMan<IOObj> ioobj = insertedobjdbky_.getIOObj();
	if ( !ioobj )
	    return false;

	seldata_.attribid_ = descset.getStoredID( ioobj->key(), 0, true );
	return true;
    }

    if ( !fully_finalised_ )
	return true;

    const SelType seltyp = selType();
    uiListBox* fld = entryList4Type( seltyp );
    if ( !fld )
	return false;
    const int selidx = fld->currentItem();
    if ( selidx < 0 )
	return false;

    if ( seltyp == Attrib )
	seldata_.attribid_ = attrinf_->attrids_[selidx];
    else if ( seltyp == NLA )
    {
	seldata_.attribid_.setInvalid();
	seldata_.setOutputNr( selidx );
    }
    else
    {
	const bool havecompsel = compfld_->mainObject()->isVisible();
	const bool haveallcomps = havecompsel
		     && compfld_->box()->textOfItem(0) == uiStrings::sAll();
	const int compselidx = havecompsel ? compfld_->box()->currentItem()
					   : 0;
	seldata_.setCompNr( haveallcomps ? compselidx-1 : compselidx );
	if ( seldata_.compNr() < 0 && !haveallcomps )
	    seldata_.setCompNr( 0 );
	const DBKey ioobjkey = seltyp==Stored
			     ? attrinf_->ioobjids_.get( selidx )
			     : attrinf_->steerids_.get( selidx );
	seldata_.attribid_ = haveallcomps && seldata_.compNr()==-1
	    ? descset.getStoredID(ioobjkey, seldata_.compNr(), true,true,"ALL")
	    : descset.getStoredID( ioobjkey, seldata_.compNr(), true );
	if ( needattrmatch && !seldata_.attribid_.isValid() )
	{
	    uiString msg = uiStrings::phrCannotFind(tr("the seismic data %1")
			 .arg(seldata_.attribid_.isInvalid()
			 ? tr("in object manager") : tr("on disk")));
	    uiMSG().error( msg );
	    return false;
	}
    }

    return true;
}


void uiAttrSelDlg::objInsertedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( DBKey, ky, cb );
    if ( ky.isValid() )
    {
	insertedobjdbky_ = ky;
	accept( 0 );
    }
}


bool uiAttrSelDlg::acceptOK()
{
    return getAttrData(true);
}



uiAttrSel::uiAttrSel( uiParent* p, const DescSet& ads, const uiString& lbltxt,
		      DescID curid )
    : uiGroup(p,"Attrib selector")
    , uiAttrSelectionObj(uiAttrSelData(ads),false)
    , lbltxt_(lbltxt)
    , selectionChanged(this)
{
    seldata_.attribid_ = curid;
    fillAttrInf();
    createFields();
}


uiAttrSel::uiAttrSel( uiParent* p, const uiAttrSelData& asd,
			const uiString& lbltxt )
    : uiGroup(p,"Attrib selector")
    , uiAttrSelectionObj(asd,false)
    , lbltxt_(lbltxt)
    , selectionChanged(this)
{
    fillAttrInf();
    createFields();
}


uiAttrSel::~uiAttrSel()
{
    detachAllNotifiers();
}


void uiAttrSel::createFields()
{
    selfld_ = new uiComboBox( this, "Attrib selector" );
    selfld_->setHSzPol( uiObject::WideVar );
    selfld_->setEditable( true );
    selfld_->setReadOnly( true );

    typfld_ = new uiComboBox( this, "Attrib type" );
    typfld_->setHSzPol( uiObject::SmallVar );
    typfld_->attach( leftOf, selfld_ );
    typfld_->addItem( selTypeDispStr(Attrib) );
	// sizing the combobox; box items will be updated later

    if ( !lbltxt_.isEmpty() )
    {
	uiLabel* lbl = new uiLabel( this, lbltxt_, typfld_ );
	lbl->setTextSelectable( false );
	if ( lbltxt_ == uiAttrSel::sQuantityToOutput()
	  && !seldata_.attribid_.isValid() )
	    seldata_.attribid_ = seldata_.attrSet().getDefaultTargetID();
    }

    uiButton* selbut = uiButton::getStd( this, OD::Select,
					 mCB(this,uiAttrSel,doSelCB), false );
    selbut->attach( rightOf, selfld_ );

    setHAlignObj( selfld_ );
    mAttachCB( postFinalise(), uiAttrSel::initFlds );
}


void uiAttrSel::initFlds( CallBacker* )
{
    putSelectionToScreen();

    mAttachCB( typfld_->selectionChanged, uiAttrSel::typSelCB );
    mAttachCB( selfld_->selectionChanged, uiAttrSel::selChgCB );

    addCBsToDescSet();

    uiAttrDescEd* ade = getParentADE();
    if ( ade )
	{ mAttachCB( ade->descSetChanged, uiAttrSel::adeDescSetChgCB ); }
}


uiAttrDescEd* uiAttrSel::getParentADE()
{
    uiParent* par = parent();
    uiParent* prevpar = par;
    while ( par && par != prevpar )
    {
	mDynamicCastGet( uiAttrDescEd*, ade, par );
	if ( ade )
	    return ade;
	uiObject* uiobj = par->mainObject();
	prevpar = par;
	par = uiobj ? uiobj->parent() : 0;
    }
    return 0;
}


void uiAttrSel::addCBsToDescSet()
{
    const DescSet& descset = seldata_.attrSet();
    mAttachCB( descset.descAdded, uiAttrSel::descSetChgCB );
    mAttachCB( descset.descUserRefChanged, uiAttrSel::descSetChgCB );
    mAttachCB( descset.descRemoved, uiAttrSel::descSetChgCB );
}


void uiAttrSel::removeCBsFromDescSet()
{
    const DescSet& descset = seldata_.attrSet();
    descset.descAdded.removeWith( this );
    descset.descUserRefChanged.removeWith( this );
    descset.descToBeRemoved.removeWith( this );
}


#define mGetTypNotifyStopper() NotifyStopper nstyp( typfld_->selectionChanged )
#define mGetSelNotifyStopper() NotifyStopper nssel( selfld_->selectionChanged )

void uiAttrSel::addTypeFldItem( SelType seltyp )
{
    typfld_->addItem( selTypeDispStr(seltyp), (int)seltyp );
    typfld_->setIcon( typfld_->size()-1, selTypeIconID(seltyp) ); \
}

void uiAttrSel::fillTypFld()
{
    mGetTypNotifyStopper();

    const int oldid = typfld_->isEmpty() ? 0 : typfld_->currentItemID();
    typfld_->setEmpty();

    addTypeFldItem( Stored );

    if ( showsteerdata_ && have(Steer) )
	addTypeFldItem( Steer );
    if ( have(Attrib) )
    {
	addTypeFldItem( Attrib );
	if ( have(NLA) )
	    addTypeFldItem( NLA );
    }

    typfld_->setCurrentItemByID( oldid );
}


void uiAttrSel::fillSelFld()
{
    mGetSelNotifyStopper();
    const BufferString oldselnm( selfld_->text() );

    const BufferStringSet* nms = 0;
    const SelType seltyp = selType();
    switch ( seltyp )
    {
	case Steer:	nms = &attrinf_->steernms_;	break;
	case Attrib:	nms = &attrinf_->attrnms_;	break;
	case NLA:	nms = &attrinf_->nlaoutnms_;	break;
	default:	{ pErrMsg("new sel type"); }	// no break
	case Stored:	nms = &attrinf_->ioobjnms_;	break;
    }

    selfld_->setEmpty();
    selfld_->addItems( *nms );
    const int idxinnms = nms->indexOf( oldselnm );
    if ( idxinnms >= 0 )
	selfld_->setText( oldselnm );
}


void uiAttrSel::getSelectionFromScreen()
{
    const int selidx = selfld_->currentItem();
    if ( selidx < 0 )
	return;

    switch ( selType() )
    {
	case Stored:
	    seldata_.attribid_ = seldata_.attrSet().getStoredID(
					    attrinf_->ioobjids_[selidx] );
	    break;
	case Steer:
	    seldata_.attribid_ = seldata_.attrSet().getStoredID(
					    attrinf_->steerids_[selidx] );
	    break;
	case Attrib:
	    seldata_.attribid_ = attrinf_->attrids_[selidx];
	    break;
	case NLA:
	    seldata_.attribid_.setInvalid();
	    seldata_.setOutputNr( selidx );
	    break;
	default:
	    pErrMsg( "New seltype defined" );
	    break;
    }
}


void uiAttrSel::putSelectionToScreen()
{
    fillTypFld();
    mGetTypNotifyStopper();

    SelType seltyp = NLA;
    const Desc* desc = 0;
    if ( !seldata_.isNLA() )
    {
	if ( !seldata_.attribid_.isValid() )
	    seldata_.attribid_ = seldata_.attrSet().ensureDefStoredPresent();

	desc = seldata_.attrSet().getDesc( seldata_.attribid_ );
	if ( !desc )
	    seltyp = Stored;
	else
	{
	    const bool isattr = !desc->isStored();
	    const bool issteer = !isattr && desc->isSteering();
	    seltyp = isattr ? Attrib : (issteer? Steer : Stored);
	}
    }

    typfld_->setCurrentItemByID( (int)seltyp );
    fillSelFld();
    if ( !desc && seltyp == Stored )
	return;

    if ( desc )
	selfld_->setCurrentItem( desc->userRef() );
    else
	selfld_->setCurrentItem( seldata_.outputNr() );
}


void uiAttrSel::updateContent( bool getnewinf, bool updatetypes )
{
    if ( getnewinf )
	fillAttrInf();
    if ( updatetypes )
	fillTypFld();
    fillSelFld();
}


void uiAttrSel::switchToDescSet( const DescSet& descset )
{
    removeCBsFromDescSet();
    seldata_.setAttrSet( descset );
    addCBsToDescSet();
}


void uiAttrSel::setDescSet( const DescSet* ads )
{
    if ( !ads || &seldata_.attrSet() == ads )
	return;

    seldata_.attribid_.setInvalid();
    switchToDescSet( *ads );
    updateContent( true );
    putSelectionToScreen();
}


void uiAttrSel::setDesc( const Desc* ad )
{
    const DescSet* descset = ad ? ad->descSet() : 0;
    if ( !descset )
	return;
    const bool isnewset = descset != &seldata_.attrSet();
    if ( !isnewset && ad->id() == seldata_.attribid_ )
	return;

    seldata_.attribid_ = ad->id();
    if ( isnewset )
	switchToDescSet( *descset );

    updateContent( isnewset );
    putSelectionToScreen();
}


void uiAttrSel::setSelSpec( const SelSpec* selspec )
{
    if ( !selspec )
	return;

    seldata_.attribid_ = selspec->id();
    updateContent();
}


void uiAttrSel::setNLAModel( const NLAModel* newmdl )
{
    if ( newmdl == seldata_.nlamodel_ )
	return;

    seldata_.nlamodel_ = newmdl;
    seldata_.setOutputNr( 0 );
    seldata_.attribid_.setInvalid();

    updateContent( true, true );
}


void uiAttrSel::showSteeringData( bool yn )
{
    if ( showsteerdata_ == yn )
	return;

    showsteerdata_ = yn;
    fillTypFld();
}


void uiAttrSel::setIgnoreDesc( const Desc* ad )
{
    const DescID newid( ad ? ad->id() : DescID() );
    if ( newid != ignoreid_ )
    {
	ignoreid_ = newid;
	updateContent( true );
    }
}


void uiAttrSel::setDataPackInputs( const DPIDSet& ids )
{
    dpids_ = ids;
    updateContent( true );
    putSelectionToScreen();
}


void uiAttrSel::typSelCB( CallBacker* )
{
    fillSelFld();
}


void uiAttrSel::selChgCB( CallBacker* )
{
    getSelectionFromScreen();
    selectionChanged.trigger();
}


void uiAttrSel::descSetChgCB( CallBacker* )
{
    updateContent( true, true );
}


void uiAttrSel::adeDescSetChgCB( CallBacker* )
{
    uiAttrDescEd* ade = getParentADE();
    if ( ade )
	setDescSet( ade->descSet() );
}


bool uiAttrSel::getRanges( TrcKeyZSampling& cs ) const
{
    if ( !seldata_.attribid_.isValid() )
	return false;
    const Desc* desc = seldata_.attrSet().getDesc( seldata_.attribid_ );
    if ( !desc )
	return false;

    const SeisIOObjInfo ioobjinfo( desc->getStoredID(true) );
    return ioobjinfo.getRanges( cs );
}


void uiAttrSel::doSelCB( CallBacker* )
{
    uiAttrSelDlg::Setup setup( lbltxt_ );
    setup.ignoreid(ignoreid_).showsteeringdata(showsteerdata_);
    uiAttrSelDlg dlg( this, seldata_, dpids_, setup );
    if ( dlg.go() )
    {
	seldata_.attribid_ = dlg.attribID();
	seldata_.setCompNr( dlg.compNr() );
	updateContent();
    }
}


uiAttrSelectionObj::SelType uiAttrSel::selType() const
{
    int cbitemid = typfld_->currentItemID();
    if ( cbitemid < 0 )
	cbitemid = 0;
    else if ( cbitemid > (int)NLA )
	{ pErrMsg("Huh"); cbitemid = 0; }
    return (SelType)cbitemid;
}


bool uiAttrSel::haveSelection() const
{
    return selfld_->currentItem() >= 0;
}


const char* uiAttrSel::getAttrName() const
{
    return selfld_->text();
}


bool uiAttrSel::isValidOutput( const IOObj& ioobj ) const
{
    if ( seldata_.isUndef() )
    {
	uiMSG().error( uiStrings::phrSelect(tr("the input")) );
	return false;
    }

    if ( is2D() && !SeisIOObjInfo(ioobj).is2D() )
    {
	uiMSG().error( tr("Can only store this in a 2D Data set") );
	return false;
    }

    return true;
}


// **** uiImagAttrSel ****

Attrib::DescID uiImagAttrSel::imagID() const
{
    const DescID selattrid = attribID();
    TypeSet<DescID> attribids;
    seldata_.attrSet().getIds( attribids );
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	const Desc* desc = seldata_.attrSet().getDesc( attribids[idx] );

	if ( desc->attribName() != Attrib::Hilbert::attribName() )
	    continue;

	const Desc* inputdesc = desc->getInput( 0 );
	if ( !inputdesc || inputdesc->id() != selattrid )
	    continue;

	return attribids[idx];
    }

    DescSet& descset = const_cast<DescSet&>(seldata_.attrSet());
    Desc* inpdesc = descset.getDesc( selattrid );
    Desc* newdesc = Attrib::PF().createDescCopy(
				Attrib::Hilbert::attribName() );
    if ( !newdesc || !inpdesc )
	return DescID();

    newdesc->selectOutput( 0 );
    newdesc->setInput( 0, inpdesc );
    newdesc->setIsHidden( true );

    BufferString usrref = "_"; usrref += inpdesc->userRef(); usrref += "_imag";
    newdesc->setUserRef( usrref );

    return descset.addDesc( newdesc );
}
