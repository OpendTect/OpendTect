/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February  2006
 RCS:           $Id: uifingerprintattrib.cc,v 1.10 2006-05-19 14:34:02 cvshelene Exp $

________________________________________________________________________

-*/

#include "uifingerprintattrib.h"
#include "fingerprintattrib.h"
#include "attribdesc.h"
#include "attribsel.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribdescset.h"
#include "attribprocessor.h"
#include "attribengman.h"
#include "uiattrsel.h"
#include "uistepoutsel.h"
#include "uiioobjsel.h"
#include "uitable.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "pixmap.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "oddirs.h"
#include "binidvalset.h"
#include "survinfo.h"
#include "transl.h"
#include "pickset.h"
#include "picksettr.h"
#include "runstat.h"
#include "uiexecutor.h"
#include "ptrman.h"
#include "stats.h"

using namespace Attrib;

static const int sInitNrRows = 4;
static const int sNrRandPicks = 100;

static const char* valinpstrs[] =
{
	"Mamual",
	"Reference Position",
	"Pickset",
	0
};


static const char* statstrs[] =
{
	"Average",
	"Median",
	"Variance",
	"Min",
	"Max",
	0
};

class uiFPAdvancedDlg: public uiDialog
{
    public:
			uiFPAdvancedDlg(uiParent*,calcFingParsObject*,
					const BufferStringSet&);
			~uiFPAdvancedDlg();
							
    void                prepareNumGroup(uiGroup*,const BufferStringSet&);
    void                rangeSel(CallBacker*);
    void                calcPush(CallBacker*);
    bool                acceptOK(CallBacker*);
    
    uiButtonGroup*      rangesgrp_;
    uiRadioButton*      picksetbut_;
    uiIOObjSel*         picksetfld_;

    ObjectSet<uiGenInput>       valflds_;
    ObjectSet<uiGenInput>       minmaxflds_;
    ObjectSet<uiSpinBox>        wgtflds_;

    calcFingParsObject& calcobj_;
    CtxtIOObj&          ctio_;
    BufferString        errmsg_;
};


uiFingerPrintAttrib::uiFingerPrintAttrib( uiParent* p )
	: uiAttrDescEd(p)
    	, ctio_(*mMkCtxtIOObj(PickSet))
{
    calcobj_ = new calcFingParsObject( this );

    refgrp_ = new uiButtonGroup( this, "Get values from", false );
    uiRadioButton* manualbut = new uiRadioButton( refgrp_, "Manual" );
    manualbut->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );
    refposbut_ = new uiRadioButton( refgrp_,"Reference position");
    refposbut_->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );
    picksetbut_ = new uiRadioButton( refgrp_, "Pickset" );
    picksetbut_->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );

    refposfld_ = new uiStepOutSel( this, "Trace position`Inl/Crl position");
    refposfld_->attach( alignedBelow, (uiParent*)refgrp_ );
    BufferString zlabel = "Z"; zlabel += SI().getZUnit();
    refposzfld_ = new uiGenInput( this, zlabel );
    refposzfld_->setElemSzPol( uiObject::Small );
    refposzfld_->attach( rightOf, refposfld_ );
    
    CallBack cb = mCB(this,uiFingerPrintAttrib,getPosPush);
    const ioPixmap pm( GetIconFileName("pick.png") );
    getposbut_ = new uiToolButton( this, "", pm, cb );
    getposbut_->attach( rightOf, refposzfld_ );

    picksetfld_ = new uiIOObjSel( this, ctio_, "Pickset file" );
    picksetfld_->attach( alignedBelow, (uiParent*)refgrp_ );
    picksetfld_->display(false);

    statsfld_ = new uiGenInput( this, "PickSet statistic", 
	    		       StringListInpSpec(statstrs) );
    statsfld_->attach( alignedBelow, picksetfld_ );
    statsfld_->display(false);
    
    manlbl_ = new uiLabel( this, 
	    		   "Please select some attributes and go to Advanced" );
    manlbl_->attach( alignedBelow, (uiParent*)refgrp_ );
    
    table_ = new uiTable( this, uiTable::Setup().rowdesc("")
						.rowgrow(true)
						.defrowlbl("")
						.fillcol(true)
						.fillrow(false) );

    const char* collbls[] = { "Reference attributes", 0 };
    table_->setColumnLabels( collbls );
    table_->setNrRows( sInitNrRows );
    table_->setColumnWidth(0,240);
    table_->setRowHeight( -1, 16 );
    table_->setToolTip( "Right-click to add, insert or remove an attribute" );
    table_->attach( alignedBelow, statsfld_ );
    table_->rowInserted.notify( mCB(this,uiFingerPrintAttrib,insertRowCB) );
    table_->rowDeleted.notify( mCB(this,uiFingerPrintAttrib,deleteRowCB) );

    CallBack cbcalc = mCB(this,uiFingerPrintAttrib,calcPush);
    uiPushButton* calcbut =
		new uiPushButton( this, "Calculate &parameters", cbcalc, true);
    calcbut->attach( alignedBelow, table_ );
    
    CallBack cbrg = mCB(this,uiFingerPrintAttrib,getAdvancedPush);
    uiPushButton* advbut = new uiPushButton( this, "&Advanced", cbrg, false );
    advbut->attach( rightAlignedBelow, table_ );

    setHAlignObj( table_ );
    refSel(0);
}


uiFingerPrintAttrib::~uiFingerPrintAttrib()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


void uiFingerPrintAttrib::initTable( int nrrows )
{
    attribflds_.erase();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	uiAttrSel* attrbox = new uiAttrSel( 0, 0, "" );
	attribflds_ += attrbox;
	table_->setCellObject( RowCol(idx,0), attrbox->attachObj() );
    }
}


void uiFingerPrintAttrib::insertRowCB( CallBacker* cb )
{
    int newrowidx = table_->currentRow();
    uiAttrSel* attrbox = new uiAttrSel( 0, 0, "" );
    attrbox->setDescSet( ads );
    attribflds_.insertAt( attrbox, newrowidx );
    table_->setCellObject( RowCol(newrowidx,0), attrbox->attachObj() );
}


void uiFingerPrintAttrib::deleteRowCB( CallBacker* cb )
{
    int deletedrowidx = table_->currentRow()<0 ? 0 : table_->currentRow()<0;
    delete attribflds_[deletedrowidx];
    attribflds_.remove( deletedrowidx );
}


void uiFingerPrintAttrib::set2D( bool yn )
{
    refposfld_->set2D( yn );
}


bool uiFingerPrintAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),FingerPrint::attribName()) )
	return false;

    mIfGetBinID( FingerPrint::refposStr(), refpos, refposfld_->setBinID(refpos))
    mIfGetFloat( FingerPrint::refposzStr(), refposz,
	    	 refposzfld_->setValue( refposz ) );

    mIfGetString( FingerPrint::valpicksetStr(), pick, 
	    	  picksetfld_->setInput(pick) )

    mIfGetInt( FingerPrint::valreftypeStr(), type, refgrp_->selectButton(type) )

    refSel(0);
    
    int nrvals = sInitNrRows;
    if ( desc.getParam( FingerPrint::valStr() ) )
    {
	TypeSet<float> values;
	mDescGetConstParamGroup(FloatParam,valueset,desc,FingerPrint::valStr())
	for ( int idx=0; idx<valueset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*valueset)[idx];
	    if ( !mIsUdf(param.getfValue(0)) )
		values += param.getfValue(0);
	}
	calcobj_->setValues( values );
	if ( valueset->size() > sInitNrRows )
	    nrvals = valueset->size();
    }

    table_->clearTable();
    table_->setNrRows( nrvals );
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
	for ( int idx=0; idx<nrvals-weights.size(); idx++ )
	    weights += 1;

	calcobj_->setWeights( weights );
    }

    mIfGetString( FingerPrint::rgpicksetStr(), rgp, calcobj_->setRgRefPick(rgp))

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
    if ( strcmp(desc.attribName(), FingerPrint::attribName()) )
	return false;

    mSetInt( FingerPrint::valreftypeStr(), refgrp_->selectedId() );
    const int refgrpval = refgrp_->selectedId();

    if ( refgrpval == 1 )
    {
	mSetBinID( FingerPrint::refposStr(), refposfld_->binID() );
	mSetFloat( FingerPrint::refposzStr(), refposzfld_->getfValue() );
    }
    else if ( refgrpval == 2 )
    {
	mSetInt( FingerPrint::statstypeStr(), statsfld_->getIntValue() );
	if ( picksetfld_->ctxtIOObj().ioobj )
	    mSetString( FingerPrint::valpicksetStr(), 
			picksetfld_->ctxtIOObj().ioobj->key().buf() )
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
	mSetString( FingerPrint::rgpicksetStr(), calcobj_->getRgRefPick() )

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
    getposbut_->display( refbutchecked );
    picksetfld_->display( pickbutchecked );
    statsfld_->display( pickbutchecked );
    manlbl_->display( !pickbutchecked && !refbutchecked );
}


void uiFingerPrintAttrib::getPosPush(CallBacker*)
{
}

void uiFingerPrintAttrib::getAdvancedPush(CallBacker*)
{
    BufferStringSet userrefset;
    for ( int idx=0; idx<table_->nrRows(); idx++ )
	if ( strcmp( "", attribflds_[idx]->getInput() ) )
	    userrefset.add( attribflds_[idx]->getInput() );

    advanceddlg_ = new uiFPAdvancedDlg( this, calcobj_, userrefset );
    
    BinIDValueSet* valuesset = createValuesBinIDSet( advanceddlg_->errmsg_ );
    calcobj_->setValRgSet( valuesset, true );
    calcobj_->setDescSet( ads );
    BufferStringSet* refset = new BufferStringSet();
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	BufferString inp = attribflds_[idx]->getInput();
	if ( inp.size() )
	    refset->add( inp );
    }
    calcobj_->setUserRefList( refset );
    
    advanceddlg_->go();
}


void uiFingerPrintAttrib::calcPush(CallBacker*)
{
    BufferString errmsg;
    BinIDValueSet* valuesset = createValuesBinIDSet( errmsg );
    if ( calcobj_->getRgRefType() ==1 && !calcobj_->getRgRefPick().size() )
    {
	errmsg = "Please choose the pickset from which\n";
	errmsg += "the ranges will be computed";
    }
    BinIDValueSet* rangesset = calcobj_->createRangesBinIDSet();
    if ( errmsg.size() ) 
    {
	uiMSG().error( errmsg );
	return;
    }
    calcobj_->setValRgSet( valuesset, true );
    calcobj_->setValRgSet( rangesset, false );
    calcobj_->setDescSet( ads );
    BufferStringSet* refset = new BufferStringSet();
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	BufferString inp = attribflds_[idx]->getInput();
	if ( inp.size() )
	    refset->add( inp );
    }
    calcobj_->setUserRefList( refset );
    calcobj_->computeValsAndRanges();
}


BinIDValueSet* uiFingerPrintAttrib::createValuesBinIDSet(
						BufferString& errmsg ) const
{
    if ( refgrp_->selectedId() == 1 )
    {
	BinID refpos_ = refposfld_->binID();
	float refposz_ = refposzfld_->getfValue() / SI().zFactor();

	if ( mIsUdf(refpos_.inl) || mIsUdf(refpos_.crl) || mIsUdf(refposz_) )
	{
	    //see if ok in 2d
	    errmsg = "Please fill in the position fields first";
	    return 0;
	}

	BinIDValueSet* bidvalset = new BinIDValueSet( 1, false );
	bidvalset->add( refpos_, refposz_ );
	return bidvalset;
    }
    else if ( refgrp_->selectedId() == 2 )
    {
	ObjectSet<BinIDValueSet> values;
	BufferString errmsg;
	picksetfld_->processInput();
	const IOObj* ioobj = picksetfld_->ctxtIOObj().ioobj;
	if ( !ioobj )
	{
	    errmsg = "Please choose the pickset from which\n";
	    errmsg += "the values will be extracted";
	    return 0;
	}
	BufferStringSet ioobjids;
	ioobjids.add( ioobj->key() );
	PickSetTranslator::createBinIDValueSets( ioobjids, values );
	return values[0];
	values.erase();
    }
    
    return new BinIDValueSet( 1, false );
}


uiFPAdvancedDlg::uiFPAdvancedDlg( uiParent* p, calcFingParsObject* calcobj,
       				  const BufferStringSet& attrrefset )
    : uiDialog( p, uiDialog::Setup("FingerPrint advanced options dialog","") )
    , ctio_(*mMkCtxtIOObj(PickSet))
    , calcobj_(*calcobj)
{
    rangesgrp_ = new uiButtonGroup( this, "Get ranges from", false );
    uiRadioButton* manualbut = new uiRadioButton( rangesgrp_, "Manual" );
    manualbut->activated.notify( mCB(this,uiFPAdvancedDlg,rangeSel ) );
    picksetbut_ = new uiRadioButton( rangesgrp_,"Pickset");
    picksetbut_->activated.notify( mCB(this,uiFPAdvancedDlg,rangeSel ) );
    uiRadioButton* autobut = new uiRadioButton( rangesgrp_, "Automatic" );
    autobut->activated.notify( mCB(this,uiFPAdvancedDlg,rangeSel ) );
    rangesgrp_->selectButton( calcobj_.getRgRefType() );

    picksetfld_ = new uiIOObjSel( this, ctio_, "Pickset file" );
    picksetfld_->attach( alignedBelow, (uiParent*)rangesgrp_ );
    picksetfld_->setInput( calcobj_.getRgRefPick() );
    picksetfld_->display( true );
    
    uiGroup* attrvalsgrp = new uiGroup( this, "Attrib inputs" );
    prepareNumGroup( attrvalsgrp, attrrefset );
	
    CallBack cbcalc = mCB(this,uiFPAdvancedDlg,calcPush);
    uiPushButton* calcbut =
	new uiPushButton( this, "Calculate &parameters", cbcalc, true);
    calcbut->attach( alignedBelow, (uiParent*)attrvalsgrp );
    
    finaliseDone.notify( mCB(this,uiFPAdvancedDlg,rangeSel) );
}


uiFPAdvancedDlg::~uiFPAdvancedDlg()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


void uiFPAdvancedDlg::prepareNumGroup( uiGroup* attrvalsgrp,
				       const BufferStringSet& attrrefset )
{
    for ( int idx=0; idx<attrrefset.size(); idx++ )
    {
	valflds_ += new uiGenInput( attrvalsgrp, attrrefset.get(idx).buf(),
				    FloatInpSpec() );
	uiSpinBox* spinbox = new uiSpinBox( attrvalsgrp );
	spinbox->setInterval( 1, 5 );
	wgtflds_ += spinbox;

	minmaxflds_ += new uiGenInput( attrvalsgrp, "", FloatInpIntervalSpec());

	wgtflds_[idx]->attach( rightOf, valflds_[idx] );
	minmaxflds_[idx]->attach( rightOf, wgtflds_[idx] );

	if ( !idx )
	{
	    uiLabel* txt = new uiLabel( attrvalsgrp, "Value" );
	    txt->attach( centeredAbove, valflds_[idx] );
	    txt = new uiLabel( attrvalsgrp, "Weight" );
	    txt->attach( centeredAbove, wgtflds_[idx] );
	    txt = new uiLabel( attrvalsgrp, "Minimum    Maximum" );
	    txt->attach( centeredAbove, minmaxflds_[idx] );
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
    if ( errmsg_.size() )\
    {\
	uiMSG().error( errmsg_ );\
	return;\
    }\

void uiFPAdvancedDlg::calcPush( CallBacker* cb )
{
    mRetIfErr;
    errmsg_.empty();
    const int refgrpval = rangesgrp_->selectedId();
    calcobj_.setRgRefType( refgrpval );

    if ( refgrpval == 1 )
    {
	picksetfld_->processInput();
	const IOObj* ioobj = picksetfld_->ctxtIOObj().ioobj;
	if ( !ioobj )
	{
	    errmsg_ = "Please choose the pickset from which\n";
	    errmsg_ += "the ranges will be computed";
	    mRetIfErr;
	}
	calcobj_.setRgRefPick( ioobj->key().buf() );
    }

    BinIDValueSet* rangesset = calcobj_.createRangesBinIDSet();
    calcobj_.setValRgSet( rangesset, false );
    calcobj_.computeValsAndRanges();

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


bool uiFPAdvancedDlg::acceptOK( CallBacker* cb )
{
    calcobj_.clearValues();
    calcobj_.clearRanges();
    calcobj_.clearWeights();

    const int refgrpval = rangesgrp_->selectedId();
    calcobj_.setRgRefType( refgrpval );

    if ( refgrpval == 1 )
    {
	if ( picksetfld_->ctxtIOObj().ioobj )
	    calcobj_.setRgRefPick( picksetfld_->ctxtIOObj().ioobj->key().buf());
    }

    TypeSet<float> values;
    TypeSet<int> weights;
    TypeSet< Interval<float> > ranges;
    for ( int idx=0; idx<valflds_.size(); idx++ )
    {
	Interval<float> range = minmaxflds_[idx]->getFInterval();
	if ( !mIsUdf(range.start) && !mIsUdf(range.stop) )
	    ranges += range;

	float val = valflds_[idx]->getfValue();
	if ( !mIsUdf(val) )
	    values += val;

	weights += wgtflds_[idx]->getValue();
    }

    if ( rangesgrp_->selectedId() == 0 && ranges.size()< valflds_.size() )
    {
	uiMSG().error("Please fill in all Min/Max values");
	return false;
    }

    calcobj_.setRanges( ranges );
    calcobj_.setValues( values );
    calcobj_.setWeights( weights );
    
    return true;
}


calcFingParsObject::calcFingParsObject( uiParent* p )
    : parent_( p )
{
    posset_.allowNull(true);
    while ( posset_.size() < 2 )
	posset_ += 0;
}


calcFingParsObject::~calcFingParsObject()
{
    deepErase(posset_);
    reflist_->deepErase();
}


void calcFingParsObject::setValRgSet( BinIDValueSet* positions, bool isvalset )
{
    delete( posset_.replace( isvalset ? 0 : 1, positions ) );
}


BinIDValueSet* calcFingParsObject::createRangesBinIDSet() const
{
    if ( rgreftype_ == 1 )
    {
	ObjectSet<BinIDValueSet> values;
	BufferStringSet ioobjids;
	ioobjids.add( getRgRefPick() );
	PickSetTranslator::createBinIDValueSets( ioobjids, values );
	return values[0];
	values.erase();
    }
    else if ( rgreftype_ == 2 )
    {
	BinIDValueSet generalset(1, true);
	BinID bid;
	StepInterval<int> irg = SI().inlRange( true );
	StepInterval<int> crg = SI().crlRange( true );
	for ( bid.inl=irg.start; bid.inl<=irg.stop; bid.inl +=irg.step )
	{
	    for ( bid.crl=crg.start; bid.crl<=crg.stop; bid.crl += crg.step )
		generalset.add( bid );
	}
	const int nrpts = generalset.totalSize();
	if ( !nrpts ) return 0;

	BinIDValueSet* rangesset = new BinIDValueSet( 2, true );
	for ( int ipt=0; ipt<sNrRandPicks; ipt++ )
	{
	    const int ptidx = Stat_getIndex( nrpts );
	    BinIDValueSet::Pos pos = generalset.getPos( ptidx );
	    float z = SI().zRange(true).start + 
				Stat_getRandom() * SI().zRange( true ).width();
	    rangesset->add( generalset.getBinID(pos), z );
	}

	return rangesset;
    }

    return new BinIDValueSet( 1, false );
}


void calcFingParsObject::computeValsAndRanges()
{
    BufferString errmsg;
    PtrMan<EngineMan> aem = createEngineMan();
    PtrMan<Processor> proc = aem->createLocationOutput( errmsg, posset_ );
    if ( !proc )
    {
	uiMSG().error(errmsg);
	return;
    }

    proc->setName("Compute reference values");
    uiExecutor dlg( parent_, *proc );
    if ( !dlg.go() )
    {
	uiMSG().error("hey");
	return;
    }

    extractAndSaveValsAndRanges();
}


void calcFingParsObject::extractAndSaveValsAndRanges()
{
    const int nrattribs = reflist_->size();
    BinIDValueSet* valueset = posset_[0];
    BinIDValueSet* rangeset = posset_[1];
    TypeSet<float> vals( nrattribs, mUdf(float) );
    TypeSet< Interval<float> > rgs( nrattribs, 
	    			    Interval<float>(mUdf(float),mUdf(float)) );

    if ( valueset->totalSize() == 1 )
    {
	const float* tmpvals = valueset->getVals( valueset->getPos(0) );
	for ( int idx=0; idx<nrattribs; idx++ )
	    vals[idx] = tmpvals[idx+1];
    }
    else if ( valueset->totalSize() > 1 )
    {
	ObjectSet< RunningStatistics<float> > statsset;
	fillInStats( valueset, statsset );
	if ( statstype_ > 1 ) // StdDev is not used
	    const_cast<calcFingParsObject*>(this)->statstype_++;
	
	for ( int idx=0; idx<nrattribs; idx++ )
	    vals[idx] = statsset[idx]->getValue((RunStats::StatType)statstype_);

	deepErase( statsset );
    }

    if ( rangeset->totalSize() >= 1 )
    {
	ObjectSet< RunningStatistics<float> > stats;
	fillInStats( rangeset, stats );
	
	for ( int idx=0; idx<nrattribs; idx++ )
	    rgs[idx] = Interval<float>( stats[idx]->min(), stats[idx]->max());

	deepErase( stats );
    }
	
    int index = 0;
    values_.erase(); ranges_.erase();
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	BufferString inp = reflist_->get(idx);
	for ( int idxdesc=0; idxdesc<attrset_->nrDescs(); idxdesc++ )
	{
	    if ( !strcmp( inp, attrset_->desc(idxdesc)->userRef() ) )
	    {
		if ( vals.size() > index )
		    values_ += vals[index];
		if ( rgs.size() > index )
		    ranges_ += rgs[index];
		index++;
	    }
	}
    }
}


EngineMan* calcFingParsObject::createEngineMan()
{
    EngineMan* aem = new EngineMan;
    
    TypeSet<SelSpec> attribspecs;
    for ( int idx=0; idx<reflist_->size(); idx++ )
    {
	for ( int idxdesc=0; idxdesc<attrset_->nrDescs(); idxdesc++ )
	{
	    if ( !strcmp( reflist_->get(idx),
			  attrset_->desc(idxdesc)->userRef() ) )
	    {
		SelSpec sp( 0, attrset_->desc(idxdesc)->id() );
		attribspecs += sp;
	    }
	}
    }

    aem->setAttribSet( attrset_ );
    aem->setAttribSpecs( attribspecs );

    return aem;
}


void calcFingParsObject::fillInStats( BinIDValueSet* bidvalset,
			ObjectSet< RunningStatistics<float> >& statsset ) const
{
    const int nrattribs = reflist_->size();
    for ( int idx=0; idx<nrattribs; idx++ )
	statsset += new RunningStatistics<float>;

    BinIDValueSet::Pos pos;
    while ( bidvalset->next(pos) )
    {
	const float* values = bidvalset->getVals( pos );
	for ( int idx=0; idx<nrattribs; idx++ )
	    *(statsset[idx]) += values[idx+1];
    }
}

