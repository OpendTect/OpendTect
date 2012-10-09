/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Payraudeau
 Date:          February  2006

________________________________________________________________________


static const char* rcsID = "$Id$";

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
#include "seis2dline.h"
#include "survinfo.h"
#include "surv2dgeom.h"
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

using namespace Attrib;

static const int cInitNrRows = 4;

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


uiFingerPrintAttrib::uiFingerPrintAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d,"101.0.5")
    , ctio_(*mMkCtxtIOObj(PickSet))
    , refposfld_(0)
    , linefld_(0)
    , getposbut_(0)
    , pickretriever_(0)
{
    calcobj_ = new calcFingParsObject( this );

    refgrp_ = new uiButtonGroup( this, "", false );
    uiRadioButton* manualbut = new uiRadioButton( refgrp_, "Manual" );
    manualbut->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );
    refposbut_ = new uiRadioButton( refgrp_,"Reference position");
    refposbut_->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );
    picksetbut_ = new uiRadioButton( refgrp_, "Pickset" );
    picksetbut_->activated.notify( mCB(this,uiFingerPrintAttrib,refSel ) );
    uiLabel* lbl = new uiLabel( this, "Get values from" );
    lbl->attach( centeredLeftOf, refgrp_ );

    refposfld_ = new uiGenInput( this,
			is2d_ ? sKey::TraceNr : "Position (Inl/Crl)",
			PositionInpSpec(PositionInpSpec::Setup(false,is2d_))
	   		.setName("Inl position",0).setName("Crl position",1) );
    refposfld_->attach( alignedBelow, refgrp_ );

    BufferString zlabel = "Z "; zlabel += SI().getZUnitString();
    refposzfld_ = new uiGenInput( this, zlabel );
    refposzfld_->setElemSzPol( uiObject::Small );
    refposzfld_->attach( rightTo, refposfld_ );

    if ( !is2d_ )
    {	
	getposbut_ = new uiToolButton( this, "pick.png", "Point in 3D scene",
			mCB(this,uiFingerPrintAttrib,getPosPush) );
	getposbut_->attach( rightOf, refposzfld_ );
	pickretriever_ = PickRetriever::getInstance();
	pickretriever_->finished()->notify(
			mCB(this,uiFingerPrintAttrib,pickRetrieved) );
    }

    if ( is2d_ )
    {
	linefld_ = new uiSeis2DLineSel( this );
	linefld_->attach( alignedBelow, refposfld_ );
    }

    picksetfld_ = new uiIOObjSel( this, ctio_, "Pickset file" );
    picksetfld_->attach( alignedBelow, refgrp_ );
    picksetfld_->display( false );

    statsfld_ = new uiGenInput( this, "PickSet statistic", 
	    		       StringListInpSpec(statstrs) );
    statsfld_->attach( alignedBelow, picksetfld_ );
    statsfld_->display( false );

    manlbl_ = new uiLabel( this, 
	    		   "Please select some attributes and go to Advanced" );
    manlbl_->attach( alignedBelow, refgrp_ );

    table_ = new uiTable( this,uiTable::Setup().rowdesc("")
					.rowgrow(true)
					.minrowhgt(1.5)
					.maxrowhgt(1.8)
					.mincolwdt(3*uiObject::baseFldSize())
					.maxcolwdt(4*uiObject::baseFldSize())
					.defrowlbl("")
					.fillcol(true)
					.fillrow(true) 
    			       ,"Reference attributes table" );

    const char* collbls[] = { "Reference attributes", 0 };
    table_->setColumnLabels( collbls );
    table_->setNrRows( cInitNrRows );
    table_->setStretch( 2, 0 );
    table_->setToolTip( "Right-click to add, insert or remove an attribute" );
    if ( linefld_ )	table_->attach( alignedBelow, linefld_ );
    else		table_->attach( alignedBelow, statsfld_ );
    table_->rowInserted.notify( mCB(this,uiFingerPrintAttrib,insertRowCB) );
    table_->rowDeleted.notify( mCB(this,uiFingerPrintAttrib,deleteRowCB) );

    BufferString str = "Right-click\nto add,\ninsert or\nremove\nan attribute";
    uiLabel* tablelab = new uiLabel( this, str.buf() );
    tablelab->attach( leftTo, table_ );

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
    if ( pickretriever_ )
	pickretriever_->finished()->remove(
			mCB(this,uiFingerPrintAttrib,pickRetrieved) );
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
	uiAttrSel* attrbox = new uiAttrSel( 0, 0, asd );
	attrbox->setLabelSelectable( false );	// reveals table right-click
						// menu underneath the label
	attrbox->setBorder( 0 );
	attribflds_ += attrbox;
	table_->setCellGroup( RowCol(idx,0), attrbox );
    }

    setAttrSelName( attribflds_ );
}


void uiFingerPrintAttrib::insertRowCB( CallBacker* cb )
{
    const int newrow = table_->newCell().row;
    const uiAttrSelData asd( is2d_, false );
    uiAttrSel* attrbox = new uiAttrSel( 0, 0, asd );
    attrbox->setLabelSelectable( false );	// reveals table right-click
    						// menu underneath the label
    attrbox->setDescSet( ads_ );
    attribflds_.insertAt( attrbox, newrow );
    table_->setCellGroup( RowCol(newrow,0), attrbox );
    
    TypeSet<int> weights = calcobj_->getWeights();
    weights.insert( newrow, 1);

    calcobj_->setWeights( weights );
    setAttrSelName( attribflds_ );
}


void uiFingerPrintAttrib::deleteRowCB( CallBacker* cb )
{
    const int row2rm = table_->notifiedCell().row;
    if ( row2rm<0 || row2rm >= attribflds_.size() )
	return;

    attribflds_.remove( row2rm );
    setAttrSelName( attribflds_ );

    TypeSet<int> weights = calcobj_->getWeights();
    weights.remove( row2rm );

    calcobj_->setWeights( weights );
}


bool uiFingerPrintAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),FingerPrint::attribName()) )
	return false;

    mIfGetBinID( FingerPrint::refposStr(), refpos,
		 is2d_ ? refposfld_->setValue(refpos.crl)
		       : refposfld_->setValue(refpos) )
    mIfGetFloat( FingerPrint::refposzStr(), refposz,
	    	 refposzfld_->setValue( refposz ) );

    if ( is2d_ )
    {
	BufferString lsnm, lnm;
	mIfGetString( FingerPrint::reflinesetStr(), ls, lsnm = ls )
	mIfGetString( FingerPrint::ref2dlineStr(), l, lnm = l )
	linefld_->set( lsnm, lnm );
    }

    mIfGetString( FingerPrint::valpicksetStr(), pickidstr, 
	    	  IOObj* ioobj = IOM().get( MultiID(pickidstr) );
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
	    if ( !mIsUdf(param.getfValue(0)) )
		values += param.getfValue(0);
	}
	calcobj_->setValues( values );

	nrvals = valueset->size()==0 ? cInitNrRows : valueset->size();
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
	mSetFloat( FingerPrint::refposzStr(), refposzfld_->getfValue() );
	mSetBinID( FingerPrint::refposStr(), refposfld_->getBinID() );
	if ( is2d_ )
	{
	    mSetString( FingerPrint::reflinesetStr(), linefld_->lineSetID() )
	    mSetString( FingerPrint::ref2dlineStr(), linefld_->lineName() )
	}
    }
    else if ( refgrpval == 2 )
    {
	mSetInt( FingerPrint::statstypeStr(), statsfld_->getIntValue() );
	if ( picksetfld_->ioobj(true) )
	    mSetString( FingerPrint::valpicksetStr(), 
			picksetfld_->key() )
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
    if ( is2d_ )
	linefld_->display( refbutchecked );
    if ( getposbut_ )
	getposbut_->display( refbutchecked );
    picksetfld_->display( pickbutchecked );
    statsfld_->display( pickbutchecked );
    manlbl_->display( !pickbutchecked && !refbutchecked );
}


void uiFingerPrintAttrib::getPosPush(CallBacker*)
{
    if ( pickretriever_ )
	pickretriever_->enable( 0 );
    if ( getposbut_ )
	getposbut_->setSensitive( false );
}


void uiFingerPrintAttrib::pickRetrieved( CallBacker* )
{
    if ( !pickretriever_ ) return;

    Coord3 crd = pickretriever_->getPos();
    if ( !is2d_ )
    {
	const BinID bid = SI().transform( crd );
	refposfld_->setValue( bid );
    }
    else
    {
// TODO: Enable when these functions are implemented in PickRetriever
//	refposfld_->setValue( pickretriever_->getTrcNr() );
//	linefld_->set( pickretriever_->getGeomID() );
    }

    refposzfld_->setValue( crd.z*SI().zDomain().userFactor() );
    getposbut_->setSensitive( true );
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
	calcobj_->setValStatsType( statsfld_->getIntValue() );
    
    advanceddlg_->go();
}


void uiFingerPrintAttrib::calcPush(CallBacker*)
{
    BufferString errmsg;
    BinIDValueSet* valuesset = createValuesBinIDSet( errmsg );
    if ( calcobj_->getRgRefType()==1 && calcobj_->getRgRefPick().isEmpty() )
    {
	errmsg = "Please choose the pickset from which\n";
	errmsg += "the ranges will be computed";
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
	calcobj_->setValStatsType( statsfld_->getIntValue() );

    calcobj_->computeValsAndRanges();
}


BinIDValueSet* uiFingerPrintAttrib::createValuesBinIDSet(
						BufferString& errmsg ) const
{
    if ( refgrp_->selectedId() == 1 )
    {
	BinID refpos = is2d_ ? get2DRefPos() : refposfld_->getBinID();
	float refposz = refposzfld_->getfValue() / SI().zFactor();

	if ( mIsUdf(refpos.inl) || mIsUdf(refpos.crl) || mIsUdf(refposz) )
	{
	    if ( is2d_ )
		errmsg = "2D lineset is not OK";
	    else
		errmsg = "Please fill in the position fields first";
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
	    errmsg = "Please choose the pickset from which\n";
	    errmsg += "the values will be extracted";
	    return 0;
	}

	BufferStringSet ioobjids;
	ioobjids.add( ioobj->key() );
	PickSetTranslator::createBinIDValueSets( ioobjids, values );
	if ( values.isEmpty() )
	{
	    errmsg = "Cannot extract values at PickSet locations."
		     " PickSet might be empty.";
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

    PtrMan<IOObj> ioobj = IOM().get( linefld_->lineSetID() );
    if ( !ioobj )
	return undef;

    BufferString fnm = ioobj->fullUserExpr(true);
    Seis2DLineSet lineset( fnm );
    S2DPOS().setCurLineSet( lineset.name() );
    for ( int idx=0 ;idx<lineset.nrLines();idx++ )
    {
	const int lineindex =
	    lineset.indexOfFirstOccurrence( linefld_->lineName() );
	if ( lineindex > -1 )
	{
	    PosInfo::Line2DData* geometry =
		new PosInfo::Line2DData( lineset.lineName(idx) );
	    if ( !S2DPOS().getGeometry(*geometry) )
		{ delete geometry; return undef; }

	    const int trcnr = refposfld_->getBinID().crl;
	    const int trcidx = geometry->indexOf( trcnr );
	    if ( geometry->positions().validIdx(trcidx) )
		return SI().transform( geometry->positions()[trcidx].coord_ );
	}
    }

    return undef;
}


bool uiFingerPrintAttrib::areUIParsOK()
{
    if ( calcobj_->getValues().isEmpty() || calcobj_->getRanges().isEmpty() )
    {
	errmsg_ = "Please fill in all values and ranges fields.\n";
	errmsg_ += "Press on 'Calculate parameters' to let OpendTect compute\n";
	errmsg_ += "them or go to 'Advanced...' to do it manually";
	return false;
    }

    return true;
}


uiFPAdvancedDlg::uiFPAdvancedDlg( uiParent* p, calcFingParsObject* calcobj,
       				  const BufferStringSet& attrrefset )
    : uiDialog( p, uiDialog::Setup("FingerPrint attribute advanced options",
				   "Specify advanced options", "101.3.2") )
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
    picksetfld_->setInput( MultiID(calcobj_.getRgRefPick().buf()) );
    picksetfld_->display( true );
    
    uiGroup* attrvalsgrp = new uiGroup( this, "Attrib inputs" );
    prepareNumGroup( attrvalsgrp, attrrefset );
	
    CallBack cbcalc = mCB(this,uiFPAdvancedDlg,calcPush);
    uiPushButton* calcbut =
	new uiPushButton( this, "Calculate &parameters", cbcalc, true);
    calcbut->attach( alignedBelow, (uiParent*)attrvalsgrp );
    
    postFinalise().notify( mCB(this,uiFPAdvancedDlg,rangeSel) );
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
	const char* attrnm = attrrefset.get(idx).buf();
	valflds_ += new uiGenInput( attrvalsgrp, attrnm,
			  FloatInpSpec().setName(BufferString("Val ",attrnm)) );
	uiSpinBox* spinbox = new uiSpinBox( attrvalsgrp );
	spinbox->setInterval( 1, 5 );
	wgtflds_ += spinbox;
	spinbox->setName( BufferString("Weight ",attrnm) );

	minmaxflds_ += new uiGenInput( attrvalsgrp, "", FloatInpIntervalSpec()
				      .setName(BufferString("Min ",attrnm),0)
				      .setName(BufferString("Max ",attrnm),1));

	wgtflds_[idx]->attach( rightOf, valflds_[idx] );
	minmaxflds_[idx]->attach( rightOf, wgtflds_[idx] );

	if ( !idx || idx == 18 )
	{
	    uiLabel* txt = new uiLabel( attrvalsgrp, "Value" );
	    txt->attach( centeredAbove, valflds_[idx] );
	    txt = new uiLabel( attrvalsgrp, "Weight" );
	    txt->attach( centeredAbove, wgtflds_[idx] );
	    txt = new uiLabel( attrvalsgrp, "Minimum    Maximum" );
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
	const IOObj* ioobj = picksetfld_->ioobj(true);
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


bool uiFPAdvancedDlg::acceptOK( CallBacker* cb )
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

