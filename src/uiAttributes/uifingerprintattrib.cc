/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February  2006

________________________________________________________________________

-*/


#include "uifingerprintattrib.h"
#include "uifingerprintcalcobj.h"
#include "fingerprintattrib.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "oddirs.h"
#include "pickretriever.h"
#include "pickset.h"
#include "picksettr.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "transl.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"
#include "uispinbox.h"
#include "uistepoutsel.h"
#include "uitable.h"
#include "od_helpids.h"

using namespace Attrib;

static const int cInitNrRows = 3;

static const char* statstrs[] =
{
	"Average",
	"Median",
	"Variance",
	"Min",
	"Max",
	0
};

mInitAttribUI(uiFingerPrintAttrib,FingerPrint,"FingerPrint",sKeyPatternGrp())


class uiFPAdvancedDlg: public uiDialog
{ mODTextTranslationClass(uiFPAdvancedDlg);
    public:
			uiFPAdvancedDlg(uiParent*,calcFingParsObject*,
					const BufferStringSet&);
			~uiFPAdvancedDlg();

    void		prepareNumGroup(uiGroup*,const BufferStringSet&);
    void		rangeSel(CallBacker*);
    void		calcPush(CallBacker*);
    bool		acceptOK(CallBacker*) override;

    uiButtonGroup*		rangesgrp_;
    uiRadioButton*		picksetbut_;
    uiIOObjSel*			picksetfld_;

    ObjectSet<uiGenInput>	valflds_;
    ObjectSet<uiGenInput>	minmaxflds_;
    ObjectSet<uiSpinBox>	wgtflds_;

    calcFingParsObject&		calcobj_;
    CtxtIOObj&			ctio_;
    uiString			errmsg_;
};


uiFingerPrintAttrib::uiFingerPrintAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d, mODHelpKey(mFingerPrintAttribHelpID) )
    , ctio_(*mMkCtxtIOObj(PickSet))
    , refposfld_(0)
    , linefld_(0)
{
    calcobj_ = new calcFingParsObject( this );

    refgrp_ = new uiButtonGroup( this, "", OD::Horizontal );
    uiRadioButton* manualbut =
		new uiRadioButton( refgrp_, uiStrings::sManual() );
    manualbut->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );

    refposbut_ = new uiRadioButton( refgrp_,tr("Reference position"));
    refposbut_->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );

    picksetbut_ = new uiRadioButton( refgrp_, uiStrings::sPointSet() );
    picksetbut_->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );

    uiLabel* lbl = new uiLabel( this, tr("Get values from") );
    lbl->attach( centeredLeftOf, refgrp_ );

    refposfld_ = new uiGenInput( this,
			is2d_ ? tr("%1 Number").arg(uiStrings::sTrace())
			: tr("Position (Inl/Crl)"),
			PositionInpSpec(PositionInpSpec::Setup(false,is2d_))
			.setName("Inl position",0).setName("Crl position",1) );
    refposfld_->attach( alignedBelow, refgrp_ );

    uiString zlabel = toUiString("%1 %2").arg(uiStrings::sZ())
					 .arg(SI().getUiZUnitString());
    refposzfld_ = new uiGenInput( this, zlabel );
    refposzfld_->setElemSzPol( uiObject::Small );
    refposzfld_->attach( rightTo, refposfld_ );

    getposbut_ = new uiToolButton( this, "pick", tr("Point in 3D scene"),
				   mCB(this,uiFingerPrintAttrib,getPosPush) );
    getposbut_->attach( rightOf, refposzfld_ );
    pickretriever_ = PickRetriever::getInstance();
    mAttachCB( pickretriever_->finished(), uiFingerPrintAttrib::pickRetrieved );

    if ( is2d_ )
    {
	linefld_ = new uiSeis2DLineSel( this );
	linefld_->attach( alignedBelow, refposfld_ );
    }

    picksetfld_ = new uiIOObjSel( this, ctio_,
			mJoinUiStrs(sPointSet(),sFile().toLower()) );
    picksetfld_->attach( alignedBelow, refgrp_ );
    picksetfld_->display( false );

    statsfld_ = new uiGenInput( this, tr("PointSet statistic"),
				StringListInpSpec(statstrs) );
    statsfld_->attach( alignedBelow, picksetfld_ );
    statsfld_->display( false );

    manlbl_ = new uiLabel( this,
		tr("Please select some attributes and go to Advanced"));
    manlbl_->attach( alignedBelow, refgrp_ );

    uiGroup* tblgrp = new uiGroup( this );
    if ( linefld_ )
	tblgrp->attach( alignedBelow, linefld_ );
    else
	tblgrp->attach( alignedBelow, statsfld_ );

    table_ = new uiTable( tblgrp, uiTable::Setup(cInitNrRows,1)
				.rowdesc(uiStrings::sAttribute())
				.defrowlbl("")
				.removeselallowed(false),
			  "Reference attributes table" );

    table_->setColumnLabel( 0, tr("Reference attribute") );
    table_->setStretch( 2, 2 );

    auto* newbut = new uiToolButton( tblgrp, "plus", tr("Add new attribute"),
			mCB(this,uiFingerPrintAttrib,insertRowCB) );
    newbut->attach( rightTo, table_ );
    auto* removebut = new uiToolButton( tblgrp, "remove", uiStrings::sRemove(),
			mCB(this,uiFingerPrintAttrib,deleteRowCB) );
    removebut->attach( alignedBelow, newbut );

    CallBack cbcalc = mCB(this,uiFingerPrintAttrib,calcPush);
    auto* calcbut =
	new uiPushButton( tblgrp, tr("Calculate parameters"), cbcalc, true);
    calcbut->attach( alignedBelow, table_ );

    CallBack cbrg = mCB(this,uiFingerPrintAttrib,getAdvancedPush);
    auto* advbut =
	new uiPushButton( tblgrp, uiStrings::sAdvanced(), cbrg, false );
    advbut->attach( rightAlignedBelow, table_ );

    setHAlignObj( table_ );
    refSel(0);
}


uiFingerPrintAttrib::~uiFingerPrintAttrib()
{
    detachAllNotifiers();
    delete ctio_.ioobj_;
    delete &ctio_;
}


static void setAttrSelName( ObjectSet<uiAttrSel>& flds )
{
    for ( int idx=0; idx<flds.size(); idx++ )
	flds[idx]->setObjectName( BufferString("Attribute ",idx) );
}


void uiFingerPrintAttrib::initTable( int nrrows )
{
    attribflds_.erase();
    const uiAttrSelData asd( is2d_, false );
    for ( int idx=0; idx<nrrows; idx++ )
    {
	uiAttrSel* attrbox = new uiAttrSel( 0, "", asd );
	attrbox->setBorder( 0 );
	attribflds_ += attrbox;
	table_->setCellGroup( RowCol(idx,0), attrbox );
    }

    setAttrSelName( attribflds_ );
}


void uiFingerPrintAttrib::insertRowCB( CallBacker* )
{
    const int newrow = table_->nrRows();
    table_->insertRows( newrow, 1 );
    const uiAttrSelData asd( is2d_, false );
    uiAttrSel* attrbox = new uiAttrSel( 0, "", asd );
    attrbox->setDescSet( ads_ );
    attribflds_.insertAt( attrbox, newrow );
    table_->setCellGroup( RowCol(newrow,0), attrbox );

    TypeSet<int> weights = calcobj_->getWeights();
    weights.insert( newrow, 1);

    calcobj_->setWeights( weights );
    setAttrSelName( attribflds_ );
}


void uiFingerPrintAttrib::deleteRowCB( CallBacker* )
{
    const int currow = table_->currentRow();
    if ( currow<0 || currow >= attribflds_.size() )
	return;

    table_->removeRow( currow );
    attribflds_.removeSingle( currow );
    setAttrSelName( attribflds_ );

    TypeSet<int> weights = calcobj_->getWeights();
    weights.removeSingle( currow );
    calcobj_->setWeights( weights );

    const int newcurrow = currow<attribflds_.size() ? currow : currow-1;
    table_->selectRow( newcurrow );
}


bool uiFingerPrintAttrib::setParameters( const Desc& desc )
{
    if ( desc.attribName() != FingerPrint::attribName() )
	return false;

    mIfGetBinID( FingerPrint::refposStr(), refpos,
		 is2d_ ? refposfld_->setValue(refpos.crl())
		       : refposfld_->setValue(refpos) )
    mIfGetFloat( FingerPrint::refposzStr(), refposz,
		 refposzfld_->setValue( refposz ) );

    if ( is2d_ )
    {
	BufferString lsnm, lnm;
//	mIfGetString( FingerPrint::reflinesetStr(), ls, lsnm = ls )
	mIfGetString( FingerPrint::ref2dlineStr(), l, lnm = l )
	linefld_->setSelLine( lnm );
    }

    mIfGetMultiID( FingerPrint::valpicksetStr(), pickid,
		  ConstPtrMan<IOObj> ioobj = IOM().get( pickid );
		  if ( ioobj ) picksetfld_->setInput( *ioobj ) );

    mIfGetInt( FingerPrint::valreftypeStr(), type, refgrp_->selectButton(type) )

    mIfGetInt( FingerPrint::statstypeStr(), statsval,
	       statsfld_->setValue(statsval) )

    refSel(0);

    int nrvals = cInitNrRows;
    if ( desc.getParam( FingerPrint::valStr() ) )
    {
	TypeSet<float> values;
	mDescGetConstParamGroup(FloatParam,valueset,desc,FingerPrint::valStr())

	for ( int idx=0; idx<valueset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*valueset)[idx];
	    if ( !mIsUdf(param.getFValue(0)) )
		values += param.getFValue(0);
	}
	calcobj_->setValues( values );

	nrvals = valueset->isEmpty() ? cInitNrRows : valueset->size();
    }

    table_->clearTable();
    while ( nrvals > table_->nrRows() )
	table_->insertRows( 0, 1 );
    while ( nrvals < table_->nrRows() )
	table_->removeRow( 0 );
    initTable( nrvals );

    if ( desc.getParam(FingerPrint::rangeStr()) )
    {
	TypeSet< Interval<float> > ranges;
	mDescGetConstParamGroup(FloatGateParam,rangeset,desc,
				FingerPrint::rangeStr());
	for ( int idx=0; idx<rangeset->size(); idx++ )
	{
	    const FloatGateParam& param = (FloatGateParam&)(*rangeset)[idx];
	    const Interval<float> rg = param.getValue();
	    if ( !mIsUdf(rg.start) && !mIsUdf(rg.stop) )
		ranges += rg;
	}
	calcobj_->setRanges( ranges );
    }

    if ( desc.getParam(FingerPrint::weightStr()) )
    {
	TypeSet<int> weights;
	mDescGetConstParamGroup(IntParam,weightset,desc,
				FingerPrint::weightStr());
	for ( int idx=0; idx<weightset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*weightset)[idx];
	    if ( !mIsUdf(param.getIntValue(0)) )
		weights += param.getIntValue(0);
	}
	const int initialwsz = weights.size();
	for ( int idx=0; idx<nrvals-initialwsz; idx++ )
	    weights += 1;

	calcobj_->setWeights( weights );
    }

    mIfGetMultiID( FingerPrint::rgpicksetStr(),
		   rgp, calcobj_->setRgRefPick(rgp))

    mIfGetInt(FingerPrint::rgreftypeStr(), rgtyp, calcobj_->setRgRefType(rgtyp))

    return true;
}


bool uiFingerPrintAttrib::setInput( const Desc& desc )
{
    const int nrflds = table_->nrRows();
    for ( int idx=0; idx<nrflds; idx++ )
	putInp( attribflds_[idx], desc, idx );

    return true;
}


bool uiFingerPrintAttrib::getParameters( Desc& desc )
{
    if ( desc.attribName() != FingerPrint::attribName() )
	return false;

    mSetInt( FingerPrint::valreftypeStr(), refgrp_->selectedId() );
    const int refgrpval = refgrp_->selectedId();

    if ( refgrpval == 1 )
    {
	mSetFloat( FingerPrint::refposzStr(), refposzfld_->getFValue() );
	mSetBinID( FingerPrint::refposStr(), refposfld_->getBinID() );
	if ( is2d_ )
	{
//	    mSetString( FingerPrint::reflinesetStr(), linefld_->lineSetID() )
	    mSetString( FingerPrint::ref2dlineStr(), linefld_->lineName() )
	}
    }
    else if ( refgrpval == 2 )
    {
	mSetInt( FingerPrint::statstypeStr(), statsfld_->getIntValue() );
	if ( picksetfld_->ioobj(true) )
	    mSetMultiID( FingerPrint::valpicksetStr(), picksetfld_->key() )
    }

    TypeSet<float> values = calcobj_->getValues();
    mDescGetParamGroup(FloatParam,valueset,desc,FingerPrint::valStr())
    valueset->setSize( values.size() );
    for ( int idx=0; idx<values.size(); idx++ )
    {
	ValParam& valparam = sCast(ValParam&,(*valueset)[idx]);
	valparam.setValue( values[idx] );
    }

    TypeSet< Interval<float> > ranges = calcobj_->getRanges();
    if ( values.size() != ranges.size() ) return false;

    mDescGetParamGroup(FloatGateParam,rangeset,desc,FingerPrint::rangeStr())
    rangeset->setSize( ranges.size() );
    for ( int idx=0; idx<ranges.size(); idx++ )
    {
	FloatGateParam& fgateparam = sCast(FloatGateParam&,(*rangeset)[idx]);
	fgateparam.setValue( ranges[idx] );
    }

    TypeSet<int> weights = calcobj_->getWeights();

    mDescGetParamGroup(IntParam,weightset,desc,FingerPrint::weightStr())
    weightset->setSize( weights.size() );
    for ( int idx=0; idx<weights.size(); idx++ )
    {
	ValParam& valparam = (ValParam&)(*weightset)[idx];
	valparam.setValue( weights[idx] );
    }

    mSetInt( FingerPrint::rgreftypeStr(), calcobj_->getRgRefType() );
    if ( calcobj_->getRgRefType() == 1 )
	mSetMultiID( FingerPrint::rgpicksetStr(), calcobj_->getRgRefPick() )

    return true;
}


bool uiFingerPrintAttrib::getInput( Desc& desc )
{
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	attribflds_[idx]->processInput();
	fillInp( attribflds_[idx], desc, idx );
    }
    return true;
}


void uiFingerPrintAttrib::refSel( CallBacker* )
{
    const bool refbutchecked = refposbut_->isChecked();
    const bool pickbutchecked = picksetbut_->isChecked();
    refposfld_->display( refbutchecked );
    refposzfld_->display( refbutchecked );
    if ( is2d_ )
	linefld_->display( refbutchecked );
    getposbut_->display( refbutchecked );
    picksetfld_->display( pickbutchecked );
    statsfld_->display( pickbutchecked );
    manlbl_->display( !pickbutchecked && !refbutchecked );
}


void uiFingerPrintAttrib::getPosPush(CallBacker*)
{
    pickretriever_->enable( 0 );
    getposbut_->setSensitive( false );
}


void uiFingerPrintAttrib::pickRetrieved( CallBacker* )
{
    if ( getposbut_->sensitive() )
	return;

    Coord3 crd = pickretriever_->getPos();
    if ( !is2d_ )
    {
	const BinID bid = SI().transform( crd );
	refposfld_->setValue( bid );
    }
    else
    {
	refposfld_->setValue( pickretriever_->getTrcNr() );
	linefld_->setSelLine( pickretriever_->getGeomID() );
    }

    refposzfld_->setValue( crd.z*SI().zDomain().userFactor() );
    getposbut_->setSensitive( true );
    pickretriever_->reset();
}


void uiFingerPrintAttrib::getAdvancedPush(CallBacker*)
{
    BufferStringSet userrefset;
    for ( int idx=0; idx<table_->nrRows(); idx++ )
	if ( !StringView(attribflds_[idx]->getInput()).isEmpty() )
	    userrefset.add( attribflds_[idx]->getInput() );

    advanceddlg_ = new uiFPAdvancedDlg( this, calcobj_, userrefset );

    BinIDValueSet* valuesset = createValuesBinIDSet( advanceddlg_->errmsg_ );
    calcobj_->setValRgSet( valuesset, true );
    calcobj_->setDescSet( ads_ );
    BufferStringSet* refset = new BufferStringSet();
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	BufferString inp = attribflds_[idx]->getInput();
	if ( inp.size() )
	    refset->add( inp );
    }
    calcobj_->setUserRefList( refset );
    if ( picksetbut_->isChecked() )
    {
	//!< Count, RMS, StdDev, NormVariance not used, so skip them
	const int statslistidx = statsfld_->getIntValue();
	int statstype = statslistidx +1;
	statstype += (statstype < (int)Stats::RMS ? 0 : 2);
	statstype += (statstype < (int)Stats::NormVariance ? 0 : 1);
	calcobj_->setValStatsType( statstype );
    }

    advanceddlg_->go();
}


void uiFingerPrintAttrib::calcPush(CallBacker*)
{
    uiString errmsg;
    BinIDValueSet* valuesset = createValuesBinIDSet( errmsg );
    if ( calcobj_->getRgRefType()==1 && calcobj_->getRgRefPick().isUdf() )
    {
	uiMSG().error( tr("Please choose the pickset from which\n"
			  "the ranges will be computed"));
    }

    if ( !errmsg.isEmpty() )
    {
	uiMSG().error( errmsg );
	return;
    }

    calcobj_->setDescSet( ads_ );
    BufferStringSet* refset = new BufferStringSet();
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	BufferString inp = attribflds_[idx]->getInput();
	if ( inp.size() )
	    refset->add( inp );
    }
    calcobj_->setUserRefList( refset );
    BinIDValueSet* rangesset = calcobj_->createRangesBinIDSet();
    calcobj_->setValRgSet( valuesset, true );
    calcobj_->setValRgSet( rangesset, false );
    if ( picksetbut_->isChecked() )
    {
	//!< Count, StdDev, Variance, NormVariance not used, so skip them
	const int statslistidx = statsfld_->getIntValue();
	int statstype = statslistidx +1;
	statstype += (statstype < (int)Stats::RMS ? 0 : 2);
	statstype += (statstype < (int)Stats::NormVariance ? 0 : 1);
	calcobj_->setValStatsType( statstype );
    }

    calcobj_->computeValsAndRanges();
}


BinIDValueSet* uiFingerPrintAttrib::createValuesBinIDSet(
						uiString& errmsg ) const
{
    if ( refgrp_->selectedId() == 1 )
    {
	BinID refpos = is2d_ ? get2DRefPos() : refposfld_->getBinID();
	float refposz = refposzfld_->getFValue() / SI().zDomain().userFactor();

	if ( mIsUdf(refpos.inl()) || mIsUdf(refpos.crl()) || mIsUdf(refposz) )
	{
	    if ( is2d_ )
		errmsg = tr("2D lineset is not OK");
	    else
		errmsg = tr("Please fill in the position fields first");
	    return 0;
	}

	BinIDValueSet* bidvalset = new BinIDValueSet( 1, false );
	bidvalset->add( refpos, refposz );
	return bidvalset;
    }
    else if ( refgrp_->selectedId() == 2 )
    {
	ObjectSet<BinIDValueSet> values;
	picksetfld_->processInput();
	const IOObj* ioobj = picksetfld_->ioobj(true);
	if ( !ioobj )
	{
	    errmsg = tr("Please choose the PointSet from which\n"
	    "the values will be extracted");
	    return 0;
	}

	BufferStringSet ioobjids;
	ioobjids.add( ioobj->key().toString() );
	PickSetTranslator::createBinIDValueSets( ioobjids, values );
	if ( values.isEmpty() )
	{
	    errmsg = tr("Cannot extract values at PointSet locations."
		     " PointSet might be empty.");
	    return 0;
	}

	BinIDValueSet* pickvals = new BinIDValueSet( *(values[0]) );
	deepErase( values );
	return pickvals;
    }

    return new BinIDValueSet( 1, false );
}



BinID uiFingerPrintAttrib::get2DRefPos() const
{
    const BinID undef( mUdf(int), mUdf(int) );
    if ( !is2d_ )
	return undef;

    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(linefld_->lineName()) );
    if ( !geom2d )
	return undef;

    const int trcnr = refposfld_->getBinID().crl();
    const int trcidx = geom2d->data().indexOf( trcnr );
    if ( geom2d->data().positions().validIdx(trcidx) )
	return SI().transform( geom2d->data().positions()[trcidx].coord_ );

    return undef;
}


bool uiFingerPrintAttrib::areUIParsOK()
{
    if ( calcobj_->getValues().isEmpty() || calcobj_->getRanges().isEmpty() )
    {
	uiMSG().error(tr("Please fill in all values and ranges fields.\n"
	          "Press on 'Calculate parameters' to let OpendTect compute\n"
	          "them or go to 'Advanced...' to do it manually"));
	return false;
    }

    return true;
}


uiFPAdvancedDlg::uiFPAdvancedDlg( uiParent* p, calcFingParsObject* calcobj,
				  const BufferStringSet& attrrefset )
    : uiDialog( p, uiDialog::Setup(tr("FingerPrint attribute advanced options"),
				   tr("Specify advanced options"),
                                   mODHelpKey(mFPAdvancedDlgHelpID) ) )
    , ctio_(*mMkCtxtIOObj(PickSet))
    , calcobj_(*calcobj)
{
    rangesgrp_ = new uiButtonGroup( this, "Get ranges from", OD::Horizontal );
    uiRadioButton* manualbut =
		new uiRadioButton( rangesgrp_, uiStrings::sManual() );
    manualbut->activated.notify( mCB(this,uiFPAdvancedDlg,rangeSel ) );
    picksetbut_ = new uiRadioButton( rangesgrp_,uiStrings::sPointSet());
    picksetbut_->activated.notify( mCB(this,uiFPAdvancedDlg,rangeSel ) );
    uiRadioButton* autobut = new uiRadioButton( rangesgrp_, tr("Automatic") );
    autobut->activated.notify( mCB(this,uiFPAdvancedDlg,rangeSel ) );
    rangesgrp_->selectButton( calcobj_.getRgRefType() );

    picksetfld_ = new uiIOObjSel( this, ctio_, mJoinUiStrs(sPointSet(),
							   sFile().toLower()) );
    picksetfld_->attach( alignedBelow, (uiParent*)rangesgrp_ );
    picksetfld_->setInput( calcobj_.getRgRefPick() );
    picksetfld_->display( true );

    uiGroup* attrvalsgrp = new uiGroup( this, "Attrib inputs" );
    prepareNumGroup( attrvalsgrp, attrrefset );

    CallBack cbcalc = mCB(this,uiFPAdvancedDlg,calcPush);
    uiPushButton* calcbut =
	new uiPushButton( this, tr("Calculate parameters"), cbcalc, true);
    calcbut->attach( alignedBelow, (uiParent*)attrvalsgrp );

    postFinalize().notify( mCB(this,uiFPAdvancedDlg,rangeSel) );
}


uiFPAdvancedDlg::~uiFPAdvancedDlg()
{
    delete ctio_.ioobj_;
    delete &ctio_;
}


void uiFPAdvancedDlg::prepareNumGroup( uiGroup* attrvalsgrp,
				       const BufferStringSet& attrrefset )
{
    for ( int idx=0; idx<attrrefset.size(); idx++ )
    {
	const char* attrnm = attrrefset.get(idx).buf();
	valflds_ += new uiGenInput( attrvalsgrp, toUiString(attrnm),
			  FloatInpSpec().setName(BufferString("Val ",attrnm)) );
	uiSpinBox* spinbox = new uiSpinBox( attrvalsgrp );
	spinbox->setInterval( 1, 5 );
	wgtflds_ += spinbox;
	spinbox->setName( BufferString("Weight ",attrnm) );

	minmaxflds_ += new uiGenInput( attrvalsgrp, uiStrings::sEmptyString(),
				       FloatInpIntervalSpec()
				      .setName(BufferString("Min ",attrnm),0)
				      .setName(BufferString("Max ",attrnm),1));

	wgtflds_[idx]->attach( rightOf, valflds_[idx] );
	minmaxflds_[idx]->attach( rightOf, wgtflds_[idx] );

	if ( !idx || idx == 18 )
	{
	    uiLabel* txt = new uiLabel( attrvalsgrp, uiStrings::sValue() );
	    txt->attach( centeredAbove, valflds_[idx] );
	    txt = new uiLabel( attrvalsgrp, tr("Weight") );
	    txt->attach( centeredAbove, wgtflds_[idx] );
	    txt = new uiLabel( attrvalsgrp, tr("Minimum    Maximum") );
	    txt->attach( centeredAbove, minmaxflds_[idx] );
	    if ( idx == 18 )
		valflds_[idx]->attach( rightOf, minmaxflds_[0] );
	}
	else
	{
	    valflds_[idx]->attach( alignedBelow, valflds_[idx-1] );
	}
    }
    attrvalsgrp->attach( alignedBelow, picksetfld_ );

    if ( calcobj_.getRanges().size() != attrrefset.size() ) return;
    if ( calcobj_.getValues().size() != attrrefset.size() ) return;

    for ( int idx=0; idx<calcobj_.getRanges().size(); idx++ )
    {
	valflds_[idx]->setValue( calcobj_.getValues()[idx] );
	minmaxflds_[idx]->setValue( ( calcobj_.getRanges()[idx] ) );
	wgtflds_[idx]->setValue( calcobj_.getWeights()[idx] );
    }
}


void uiFPAdvancedDlg::rangeSel( CallBacker* )
{
    picksetfld_->display( picksetbut_->isChecked() );
}


#define mRetIfErr \
    if ( !errmsg_.isEmpty() )\
    {\
	uiMSG().error( errmsg_ );\
	return;\
    }\

void uiFPAdvancedDlg::calcPush( CallBacker* )
{
    mRetIfErr;
    errmsg_.setEmpty();
    const int refgrpval = rangesgrp_->selectedId();
    calcobj_.setRgRefType( refgrpval );

    if ( refgrpval == 1 )
    {
	picksetfld_->processInput();
	const IOObj* ioobj = picksetfld_->ioobj(true);
	if ( !ioobj )
	{
	    errmsg_ = tr("Please choose the PointSet from which\n"
			 "the ranges will be computed");
	    mRetIfErr;
	}
	calcobj_.setRgRefPick( ioobj->key() );
    }

    BinIDValueSet* rangesset = calcobj_.createRangesBinIDSet();
    calcobj_.setValRgSet( rangesset, false );
    if ( !calcobj_.computeValsAndRanges() )
	return;

    if ( calcobj_.getValues()[0] && !mIsUdf( calcobj_.getValues()[0] ) )
    {
	for( int idx=0; idx<calcobj_.getValues().size(); idx++ )
	    valflds_[idx]->setValue( calcobj_.getValues()[idx] );
    }

    if ( rangesgrp_->selectedId() != 0 )
    {
	for( int idx=0; idx<calcobj_.getRanges().size(); idx++ )
	    minmaxflds_[idx]->setValue( calcobj_.getRanges()[idx] );
    }
}


bool uiFPAdvancedDlg::acceptOK( CallBacker* )
{
    calcobj_.clearValues();
    calcobj_.clearRanges();
    calcobj_.clearWeights();

    const int refgrpval = rangesgrp_->selectedId();
    calcobj_.setRgRefType( refgrpval );

    if ( refgrpval == 1 )
    {
	if ( picksetfld_->ioobj(true) )
	    calcobj_.setRgRefPick( picksetfld_->key() );
    }

    TypeSet<float> values;
    TypeSet<int> weights;
    TypeSet< Interval<float> > ranges;
    for ( int idx=0; idx<valflds_.size(); idx++ )
    {
	Interval<float> range = minmaxflds_[idx]->getFInterval();
	if ( !mIsUdf(range.start) && !mIsUdf(range.stop) )
	    ranges += range;

	float val = valflds_[idx]->getFValue();
	if ( !mIsUdf(val) )
	    values += val;

	weights += wgtflds_[idx]->getIntValue();
    }

    if ( rangesgrp_->selectedId() == 0 && ranges.size()< valflds_.size() )
    {
	uiMSG().error(tr("Please fill in all Min/Max values"));
	return false;
    }

    calcobj_.setRanges( ranges );
    calcobj_.setValues( values );
    calcobj_.setWeights( weights );

    return true;
}
