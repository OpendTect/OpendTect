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
#include "binnedvalueset.h"
#include "ioobjctxt.h"
#include "ioobj.h"
#include "oddirs.h"
#include "pickretriever.h"
#include "picksetmanager.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "transl.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uipicksetsel.h"
#include "uitoolbutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"
#include "uispinbox.h"
#include "uistepoutsel.h"
#include "uitable.h"
#include "od_helpids.h"

using namespace Attrib;

static const int cInitNrRows = 4;

static uiWord sDispName()
{
    return od_static_tr("sDispName","Finger Attribute");
}

mInitGrpDefAttribUINoSynth( uiFingerPrintAttrib, FingerPrint, sDispName(),
							    sPatternGrp() );


class uiFPAdvancedDlg: public uiDialog
{ mODTextTranslationClass(uiFPAdvancedDlg);
    public:

			uiFPAdvancedDlg(uiParent*,calcFingParsObject*,
					const BufferStringSet&);

    void		prepareNumGroup(uiGroup*,const BufferStringSet&);
    void		rangeSel(CallBacker*);
    void		calcPush(CallBacker*);
    bool		acceptOK();

    uiButtonGroup*	rangesgrp_;
    uiRadioButton*	picksetbut_;
    uiPickSetIOObjSel*	picksetfld_;

    ObjectSet<uiGenInput> valflds_;
    ObjectSet<uiGenInput> minmaxflds_;
    ObjectSet<uiSpinBox> wgtflds_;

    calcFingParsObject&	calcobj_;
    uiString		errmsg_;
};


uiFingerPrintAttrib::uiFingerPrintAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d, mODHelpKey(mFingerPrintAttribHelpID) )
    , refposfld_(0)
    , linefld_(0)
    , def_ ( *(new EnumDefImpl<Stats::Type> (Stats::TypeDef() ) ))
{
    calcobj_ = new calcFingParsObject( this );

    refgrp_ = new uiButtonGroup( this, "", OD::Horizontal );
    uiRadioButton* manualbut = new uiRadioButton( refgrp_,
                                                  uiStrings::sManual() );
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

    uiString zlabel = toUiString("%1 (%2)").arg(uiStrings::sZ())
					   .arg(SI().zUnitString());
    refposzfld_ = new uiGenInput( this, zlabel );
    refposzfld_->setElemSzPol( uiObject::Small );
    refposzfld_->attach( rightTo, refposfld_ );

    getposbut_ = new uiToolButton( this, "pick", tr("Point in 3D scene"),
				   mCB(this,uiFingerPrintAttrib,getPosPush) );
    getposbut_->attach( rightOf, refposzfld_ );
    pickretriever_ = PickRetriever::getInstance();
    pickretriever_->finished()->notify(
			mCB(this,uiFingerPrintAttrib,pickRetrieved) );

    if ( is2d_ )
    {
	linefld_ = new uiSeis2DLineSel( this );
	linefld_->attach( alignedBelow, refposfld_ );
    }

    picksetfld_ = new uiPickSetIOObjSel( this );
    picksetfld_->attach( alignedBelow, refgrp_ );
    picksetfld_->display( false );

    def_.remove( def_.getKey(Stats::Count) );
    def_.remove( def_.getKey(Stats::RMS) );
    def_.remove( def_.getKey(Stats::StdDev) );
    def_.remove( def_.getKey(Stats::NormVariance) );
    def_.remove( def_.getKey(Stats::Extreme) );
    def_.remove( def_.getKey(Stats::Sum) );
    def_.remove( def_.getKey(Stats::SqSum) );
    def_.remove( def_.getKey(Stats::MostFreq) );

    statsfld_ = new uiComboBox( this, def_, "PointSet statistic" );
    statsfld_->attach( alignedBelow, picksetfld_ );
    statsfld_->display( false );

    manlbl_ = new uiLabel( this, uiStrings::phrSelect(tr(
					"some attributes and go to Advanced")));
    manlbl_->attach( alignedBelow, refgrp_ );

    uiGroup* tblgrp = new uiGroup( this );
    if ( linefld_ )	tblgrp->attach( alignedBelow, linefld_ );
    else		tblgrp->attach( alignedBelow, statsfld_ );
    table_ = new uiTable( tblgrp, uiTable::Setup()
				.rowdesc(uiStrings::sAttribute())
				.rowgrow(true)
				.minrowhgt(1.5)
				.maxrowhgt(1.8)
				.defrowlbl("")
				.fillcol(true)
				.fillrow(true)
				.removeselallowed(false)
				.mincolwdt(3.f*uiObject::baseFldSize())
				.maxcolwdt(4.f*uiObject::baseFldSize()),
			  "Reference attributes table" );

    table_->setColumnLabel( 0, tr("Reference attribute") );
    table_->setNrRows( cInitNrRows );
    table_->setStretch( 2, 0 );
    table_->setToolTip(tr("Right-click to add, insert or remove an attribute"));
    table_->rowInserted.notify( mCB(this,uiFingerPrintAttrib,insertRowCB) );
    table_->rowDeleted.notify( mCB(this,uiFingerPrintAttrib,deleteRowCB) );

    uiString str = tr("Right-click\nto add,\ninsert or\nremove\nan attribute");
    uiLabel* tablelab = new uiLabel( tblgrp, str );
    tablelab->attach( leftTo, table_ );

    CallBack cbcalc = mCB(this,uiFingerPrintAttrib,calcPush);
    uiPushButton* calcbut =
		new uiPushButton( tblgrp, tr("Calculate parameters"),
                                  cbcalc, true);
    calcbut->attach( alignedBelow, table_ );

    CallBack cbrg = mCB(this,uiFingerPrintAttrib,getAdvancedPush);
    uiPushButton* advbut = new uiPushButton( tblgrp, uiStrings::sAdvanced(),
                                             cbrg, false );
    advbut->attach( rightAlignedBelow, table_ );

    setHAlignObj( table_ );
    refSel(0);
}


uiFingerPrintAttrib::~uiFingerPrintAttrib()
{
    pickretriever_->finished()->remove(
			mCB(this,uiFingerPrintAttrib,pickRetrieved) );
}


void uiFingerPrintAttrib::initTable( int nrrows )
{
    attribflds_.erase();
    const uiAttrSelData asd( is2d_ );
    for ( int idx=0; idx<nrrows; idx++ )
    {
	uiAttrSel* attrbox = new uiAttrSel( 0, asd, uiString::empty() );
	attrbox->setBorder( 0 );
	attribflds_ += attrbox;
	table_->setCellGroup( RowCol(idx,0), attrbox );
    }
}


void uiFingerPrintAttrib::insertRowCB( CallBacker* cb )
{
    const int newrow = table_->newCell().row();
    const uiAttrSelData asd( is2d_ );
    uiAttrSel* attrbox = new uiAttrSel( 0, asd, uiString::empty() );
    attrbox->setDescSet( ads_ );
    attribflds_.insertAt( attrbox, newrow );
    table_->setCellGroup( RowCol(newrow,0), attrbox );

    TypeSet<int> weights = calcobj_->getWeights();
    weights.insert( newrow, 1);

    calcobj_->setWeights( weights );
}


void uiFingerPrintAttrib::deleteRowCB( CallBacker* cb )
{
    const int row2rm = table_->notifiedCell().row();
    if ( row2rm<0 || row2rm >= attribflds_.size() )
	return;

    attribflds_.removeSingle( row2rm );

    TypeSet<int> weights = calcobj_->getWeights();
    weights.removeSingle( row2rm );

    calcobj_->setWeights( weights );
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
	mIfGetString( FingerPrint::ref2dlineStr(), l, lnm = l )
	linefld_->setSelLine( lnm );
    }

    mIfGetString( FingerPrint::valpicksetStr(), pickidstr,
		  IOObj* ioobj = DBKey(pickidstr).getIOObj();
		  if ( ioobj ) picksetfld_->setInput( *ioobj ) );

    mIfGetInt( FingerPrint::valreftypeStr(), type, refgrp_->selectButton(type) )

    Attrib::ValParam* valparamstatsval = const_cast<Attrib::ValParam*>(
				desc.getValParam(FingerPrint::statstypeStr()));
    mDynamicCastGet(Attrib::EnumParam*,enumparamstatsval,valparamstatsval);
    if ( enumparamstatsval )
    {
	int statsval;
	if ( enumparamstatsval->isSet() )
	    statsval = enumparamstatsval->getIntValue(0);
	else
	    statsval = enumparamstatsval->getDefaultIntValue(0);
	statsfld_->setCurrentItem( FingerPrint::getStatsTypeString(statsval) );
    }
    else
    {
	//!< Old param files, prior to 6.2
	int statstype = valparamstatsval->getIntValue(0) + 1;
	statstype += (statstype < (int)Stats::RMS ? 0 : 2);
	statstype += (statstype < (int)Stats::NormVariance ? 0 : 1);
	//!< Count, RMS, StdDev, NormVariance not used, so skip them
	statsfld_->setCurrentItem(Stats::TypeDef().getKeyForIndex(statstype) );
    }

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

    if ( desc.getParam( FingerPrint::rangeStr() ) )
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

    if ( desc.getParam( FingerPrint::weightStr() ) )
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

    mIfGetString( FingerPrint::rgpicksetStr(), rgp,
		calcobj_->setRgRefPick(DBKey(rgp)))

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
	    mSetString( FingerPrint::ref2dlineStr(), linefld_->lineName() )
	}
    }
    else if ( refgrpval == 2 )
    {
	mSetEnum( FingerPrint::statstypeStr(),
		  Stats::TypeDef().indexOf( statsfld_->currentItem() ) );
	if ( picksetfld_->ioobj(true) )
	    mSetString( FingerPrint::valpicksetStr(),
			picksetfld_->key().toString() )
    }

    TypeSet<float> values = calcobj_->getValues();
    mDescGetParamGroup(FloatParam,valueset,desc,FingerPrint::valStr())
    valueset->setSize( values.size() );
    for ( int idx=0; idx<values.size(); idx++ )
    {
	ValParam& valparam = (ValParam&)(*valueset)[idx];
	valparam.setValue( values[idx] );
    }

    TypeSet< Interval<float> > ranges = calcobj_->getRanges();
    if ( values.size() != ranges.size() ) return false;

    mDescGetParamGroup(FloatGateParam,rangeset,desc,FingerPrint::rangeStr())
    rangeset->setSize( ranges.size() );
    for ( int idx=0; idx<ranges.size(); idx++ )
    {
	FloatGateParam& fgateparam = (FloatGateParam&)(*rangeset)[idx];
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
	mSetString( FingerPrint::rgpicksetStr(),
		    calcobj_->getRgRefPick().toString() )

    return true;
}


uiRetVal uiFingerPrintAttrib::getInput( Desc& desc )
{
    uiRetVal uirv;
    for ( int idx=0; idx<attribflds_.size(); idx++ )
	uirv.add( fillInp(attribflds_[idx],desc,idx) );
    return uirv;
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
    Coord3 crd = pickretriever_->getPos();
    if ( !is2d_ )
    {
	const BinID bid = SI().transform( crd.getXY() );
	refposfld_->setValue( bid );
    }
    else
    {
	refposfld_->setValue( pickretriever_->getTrcNr() );
	linefld_->setSelLine( pickretriever_->getGeomID() );
    }

    refposzfld_->setValue( crd.z_*SI().zDomain().userFactor() );
    getposbut_->setSensitive( true );
    pickretriever_->reset();
}


void uiFingerPrintAttrib::getAdvancedPush(CallBacker*)
{
    BufferStringSet userrefset;
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	const BufferString attrnm( attribflds_[idx]->getAttrName() );
	if ( !attrnm.isEmpty() )
	    userrefset.add( attrnm );
    }

    advanceddlg_ = new uiFPAdvancedDlg( this, calcobj_, userrefset );

    BinnedValueSet* valuesset = createValuesBinIDSet( advanceddlg_->errmsg_ );
    calcobj_->setValRgSet( valuesset, true );
    calcobj_->setDescSet( ads_ );
    BufferStringSet* refset = new BufferStringSet();
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	const BufferString attrnm = attribflds_[idx]->getAttrName();
	if ( !attrnm.isEmpty() )
	    refset->add( attrnm );
    }
    calcobj_->setUserRefList( refset );
    if ( picksetbut_->isChecked() )
	calcobj_->setValStatsType(
			def_.getEnumForIndex(statsfld_->currentItem() ) );

    advanceddlg_->go();
}


void uiFingerPrintAttrib::calcPush(CallBacker*)
{
    uiString errmsg;
    BinnedValueSet* valuesset = createValuesBinIDSet( errmsg );
    if ( calcobj_->getRgRefType()==1 && calcobj_->getRgRefPick().isInvalid() )
    {
	uiMSG().error(uiStrings::phrSelect(tr("the pickset from which\n"
	                 "the ranges will be computed")));
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
	const BufferString attrnm = attribflds_[idx]->getAttrName();
	if ( !attrnm.isEmpty() )
	    refset->add( attrnm );
    }
    calcobj_->setUserRefList( refset );
    BinnedValueSet* rangesset = calcobj_->createRangesBinIDSet();
    calcobj_->setValRgSet( valuesset, true );
    calcobj_->setValRgSet( rangesset, false );
    if ( picksetbut_->isChecked() )
	calcobj_->setValStatsType( Stats::TypeDef().parse(statsfld_->text() ) );

    calcobj_->computeValsAndRanges();
}


BinnedValueSet* uiFingerPrintAttrib::createValuesBinIDSet(
						uiString& errmsg ) const
{
    BinnedValueSet* retset = new BinnedValueSet( 1, false );
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

	retset->add( refpos, refposz );
    }
    else if ( refgrp_->selectedId() == 2 )
    {
	const IOObj* ioobj = picksetfld_->ioobj(true);
	if ( !ioobj )
	{
	    errmsg = tr("Please choose the pickset from which\n"
	    "the values will be extracted");
	    return 0;
	}

	uiRetVal uirv;
	ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( ioobj->key(), uirv );
	if ( !ps )
	    { errmsg = uirv; return 0; }

	Pick::SetIter psiter( *ps );
	while ( psiter.next() )
	{
	    const Pick::Location& pl = psiter.get();
	    retset->add( pl.binID(), pl.z() );
	}
    }

    return retset;
}



BinID uiFingerPrintAttrib::get2DRefPos() const
{
    static const BinID undef( mUdf(int), mUdf(int) );
    if ( !is2d_ )
	return undef;

    const auto& geom2d = SurvGeom::get2D( linefld_->lineName() );
    if ( geom2d.isEmpty() )
	return undef;

    const int trcnr = refposfld_->getBinID().crl();
    const int trcidx = geom2d.data().indexOf( trcnr );
    if ( trcidx >= 0 )
	return SI().transform( geom2d.data().positions()[trcidx].coord_ );

    return undef;
}


uiRetVal uiFingerPrintAttrib::areUIParsOK()
{
    if ( calcobj_->getValues().isEmpty() || calcobj_->getRanges().isEmpty() )
	return uiRetVal( tr("Please fill in all values and ranges fields.\n"
	          "Press on 'Calculate parameters' to let OpendTect compute\n"
	          "them or go to 'Advanced...' to do it manually") );

    return uiRetVal::OK();
}


uiFPAdvancedDlg::uiFPAdvancedDlg( uiParent* p, calcFingParsObject* calcobj,
				  const BufferStringSet& attrrefset )
    : uiDialog( p, uiDialog::Setup(tr("FingerPrint attribute advanced options"),
				   tr("Specify advanced options"),
                                   mODHelpKey(mFPAdvancedDlgHelpID) ) )
    , calcobj_(*calcobj)
{
    rangesgrp_ = new uiButtonGroup( this, "Get ranges from", OD::Horizontal );
    uiRadioButton* manualbut = new uiRadioButton( rangesgrp_,
                                                  uiStrings::sManual() );
    manualbut->activated.notify( mCB(this,uiFPAdvancedDlg,rangeSel ) );
    picksetbut_ = new uiRadioButton( rangesgrp_,uiStrings::sPointSet());
    picksetbut_->activated.notify( mCB(this,uiFPAdvancedDlg,rangeSel ) );
    uiRadioButton* autobut = new uiRadioButton( rangesgrp_,
					        uiStrings::sAutomatic() );
    autobut->activated.notify( mCB(this,uiFPAdvancedDlg,rangeSel ) );
    rangesgrp_->selectButton( calcobj_.getRgRefType() );

    picksetfld_ = new uiPickSetIOObjSel( this );
    picksetfld_->attach( alignedBelow, (uiParent*)rangesgrp_ );
    picksetfld_->setInput( calcobj_.getRgRefPick() );
    picksetfld_->display( true );

    uiGroup* attrvalsgrp = new uiGroup( this, "Attrib inputs" );
    prepareNumGroup( attrvalsgrp, attrrefset );

    CallBack cbcalc = mCB(this,uiFPAdvancedDlg,calcPush);
    uiPushButton* calcbut =
	new uiPushButton( this, tr("Calculate parameters"), cbcalc, true);
    calcbut->attach( alignedBelow, (uiParent*)attrvalsgrp );

    postFinalise().notify( mCB(this,uiFPAdvancedDlg,rangeSel) );
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

	minmaxflds_ += new uiGenInput( attrvalsgrp, uiString::empty(),
				       FloatInpIntervalSpec()
				      .setName(BufferString("Min ",attrnm),0)
				      .setName(BufferString("Max ",attrnm),1));

	wgtflds_[idx]->attach( rightOf, valflds_[idx] );
	minmaxflds_[idx]->attach( rightOf, wgtflds_[idx] );

	if ( !idx || idx == 18 )
	{
	    uiLabel* txt = new uiLabel( attrvalsgrp, uiStrings::sValue() );
	    txt->attach( centeredAbove, valflds_[idx] );
	    txt = new uiLabel( attrvalsgrp, uiStrings::sWeight() );
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

void uiFPAdvancedDlg::calcPush( CallBacker* cb )
{
    mRetIfErr;
    errmsg_.setEmpty();
    const int refgrpval = rangesgrp_->selectedId();
    calcobj_.setRgRefType( refgrpval );

    if ( refgrpval == 1 )
    {
	const IOObj* ioobj = picksetfld_->ioobj(true);
	if ( !ioobj )
	{
	    errmsg_ = tr("Please choose the pickset from which\n"
			 "the ranges will be computed");
	    mRetIfErr;
	}
	calcobj_.setRgRefPick( ioobj->key() );
    }

    BinnedValueSet* rangesset = calcobj_.createRangesBinIDSet();
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


bool uiFPAdvancedDlg::acceptOK()
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
