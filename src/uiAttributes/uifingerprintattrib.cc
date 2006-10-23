/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February  2006
 RCS:           $Id: uifingerprintattrib.cc,v 1.24 2006-10-23 09:16:50 cvshelene Exp $

________________________________________________________________________

-*/

#include "uifingerprintattrib.h"
#include "uifingerprintcalcobj.h"
#include "fingerprintattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "attribdescset.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uistepoutsel.h"
#include "uiioobjsel.h"
#include "uitable.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uispinbox.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "pixmap.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "oddirs.h"
#include "binidvalset.h"
#include "survinfo.h"
#include "transl.h"
#include "pickset.h"
#include "picksettr.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "uiseissel.h"
#include "uiseisioobjinfo.h"
#include "seis2dline.h"
#include "segposinfo.h"
#include "ptrman.h"

using namespace Attrib;

static const int sInitNrRows = 4;

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

mInitAttribUI(uiFingerPrintAttrib,FingerPrint,"FingerPrint",sKeyPatternGrp)


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
    
    getposbut_ = new uiToolButton( this, "", ioPixmap("pick.png"),
	    			   mCB(this,uiFingerPrintAttrib,getPosPush) );
    getposbut_->attach( rightOf, refposzfld_ );

    linesetfld_ = new uiGenInput( this, "LineSet", StringInpSpec() );
    linesetfld_-> attach( alignedBelow, refposfld_ );

    CallBack sel2dcb = mCB(this,uiFingerPrintAttrib,fillIn2DPos);
    sel2dbut_ = new uiPushButton( this, "&Select", sel2dcb, false);
    sel2dbut_->attach( rightOf, linesetfld_ );

    linefld_ = new uiLabeledComboBox( this, "Line name" );
    linefld_-> attach( alignedBelow, linesetfld_ );

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
    
    table_ = new uiTable( this,uiTable::Setup().rowdesc("")
					.rowgrow(true)
					.minrowhgt(1.5)
					.maxrowhgt(1.8)
					.mincolwdt(3*uiObject::baseFldSize())
					.maxcolwdt(4*uiObject::baseFldSize())
					.defrowlbl("")
					.fillcol(true)
					.fillrow(true) );

    const char* collbls[] = { "Reference attributes", 0 };
    table_->setColumnLabels( collbls );
    table_->setNrRows( sInitNrRows );
    table_->setColumnWidth(0,240);
    table_->setRowHeight( -1, 4 );
    table_->setStretch( 0, 0 );
    table_->setPrefHeightInChar( 8 );
    table_->setToolTip( "Right-click to add, insert or remove an attribute" );
    table_->attach( alignedBelow, linefld_ );
    table_->rowInserted.notify( mCB(this,uiFingerPrintAttrib,insertRowCB) );
    table_->rowDeleted.notify( mCB(this,uiFingerPrintAttrib,deleteRowCB) );

    BufferString lbl = "Right-click\nto add,\ninsert or\nremove\nan attribute";
    uiLabel* tablelab = new uiLabel( this, lbl.buf() );
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
    const int newrow = table_->newCell().row;
    uiAttrSel* attrbox = new uiAttrSel( 0, 0, "" );
    attrbox->setDescSet( ads_ );
    attribflds_.insertAt( attrbox, newrow );
    table_->setCellObject( RowCol(newrow,0), attrbox->attachObj() );
    
    TypeSet<int> weights = calcobj_->getWeights();
    weights.insert( newrow, 1);

    calcobj_->setWeights( weights );
}


void uiFingerPrintAttrib::deleteRowCB( CallBacker* cb )
{
    const int row2rm = table_->notifiedCell().row;
    if ( row2rm<0 || row2rm >= attribflds_.size() )
	return;

    delete attribflds_[row2rm];
    attribflds_.remove( row2rm );
    
    TypeSet<int> weights = calcobj_->getWeights();
    weights.remove( row2rm );

    calcobj_->setWeights( weights );
}


void uiFingerPrintAttrib::set2D( bool yn )
{
    refposfld_->set2D( yn );
    const bool needposflds = refgrp_->selectedId() == 1;
    linesetfld_->display( yn && needposflds );
    linefld_->display( yn && needposflds );
    sel2dbut_->display( yn && needposflds );
}


bool uiFingerPrintAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),FingerPrint::attribName()) )
	return false;

    mIfGetBinID( FingerPrint::refposStr(), refpos, refposfld_->setBinID(refpos))
    mIfGetFloat( FingerPrint::refposzStr(), refposz,
	    	 refposzfld_->setValue( refposz ) );

    mIfGetString( FingerPrint::reflinesetStr(), ls, useLineSetID( ls ) )

    mIfGetString( FingerPrint::ref2dlineStr(), line, 
	    	  linefld_->box()->setCurrentItem(line.buf()) )

    mIfGetString( FingerPrint::valpicksetStr(), pickidstr, 
	    	  IOObj* ioobj = IOM().get( MultiID(pickidstr) );
		  picksetfld_->ctxtIOObj().setObj( ioobj );
	   	  picksetfld_->updateInput() )

    mIfGetInt( FingerPrint::valreftypeStr(), type, refgrp_->selectButton(type) )

    mIfGetInt( FingerPrint::statstypeStr(), statsval,
	       statsfld_->setValue(statsval) )

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
	mSetBinID( FingerPrint::refposStr(), refposfld_->binID() );
	mSetFloat( FingerPrint::refposzStr(), refposzfld_->getfValue() );
	if ( desc.descSet() && desc.descSet()->is2D() )
	{
	    mSetString( FingerPrint::reflinesetStr(), lsid_ )
	    mSetString( FingerPrint::ref2dlineStr(), linefld_->box()->text() )
	}
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
    const bool is2d = refposfld_->is2D();
    refposfld_->display( refbutchecked );
    refposzfld_->display( refbutchecked );
    linesetfld_->display( refbutchecked && is2d );
    linefld_->display( refbutchecked && is2d );
    sel2dbut_->display( refbutchecked && is2d );
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
    if ( calcobj_->getRgRefType()==1 && !calcobj_->getRgRefPick().size() )
    {
	errmsg = "Please choose the pickset from which\n";
	errmsg += "the ranges will be computed";
    }
    if ( errmsg.size() ) 
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
	BinID refpos = refposfld_->is2D() ? get2DRefPos() : refposfld_->binID();
	float refposz = refposzfld_->getfValue() / SI().zFactor();

	if ( mIsUdf(refpos.inl) || mIsUdf(refpos.crl) || mIsUdf(refposz) )
	{
	    if ( refposfld_->is2D() )
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


void uiFingerPrintAttrib::fillIn2DPos(CallBacker*)
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    SeisSelSetup setup;
    setup.pol2d( Only2D ).selattr( false );
    uiSeisSelDlg dlg( this, *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return;

    lsid_ = dlg.ioObj()->key();
    BufferString lsname;
    get2DLineSetName ( lsid_, lsname );
    linesetfld_->setText( lsname );
    BufferStringSet linenames;
    uiSeisIOObjInfo objinfo( lsid_ );
    objinfo.getLineNames( linenames );
    linefld_->box()->empty();
    for  ( int idx=0; idx<linenames.size();idx++ )
	linefld_->box()->addItem( linenames.get(idx).buf() );
}


#define mGet2DLineSet(mid,retval) \
    PtrMan<IOObj> ioobj = IOM().get( mid ); \
    if ( !ioobj ) return retval; \
    BufferString fnm = ioobj->fullUserExpr(true); \
    Seis2DLineSet lineset( fnm );


void uiFingerPrintAttrib::get2DLineSetName( const MultiID& mid,
					    BufferString& setname ) const
{
    mGet2DLineSet(mid,)
    setname = lineset.name();
}


BinID uiFingerPrintAttrib::get2DRefPos() const
{
    mGet2DLineSet( lsid_, BinID(mUdf(int),mUdf(int)) );
    for ( int idx=0 ;idx<lineset.nrLines();idx++ )
    {
	LineKey lkey( linefld_->box()->text(), "Seis" );
	const int lineindex = lineset.indexOf(lkey);
	if ( lineindex > -1 )
	{
	    PosInfo::Line2DData* geometry = new PosInfo::Line2DData;
	    if ( !lineset.getGeometry(lineindex,*geometry) )
	    {
		delete geometry;
		return BinID(mUdf(int),mUdf(int));
	    }
	    StepInterval<int> trcrg;
	    lineset.getRanges( lineindex, trcrg, geometry->zrg );
	    const int trcnr = refposfld_->binID().crl;
	    return SI().transform( geometry->posns[trcnr-trcrg.start].coord );
	}
    }
    return BinID(mUdf(int),mUdf(int));
}


void uiFingerPrintAttrib::useLineSetID( const BufferString& ls )
{
    lsid_ = MultiID( ls );
    BufferString lsname;
    get2DLineSetName ( lsid_, lsname );
    linesetfld_->setText( lsname );
    BufferStringSet linenames;
    uiSeisIOObjInfo objinfo( lsid_ );
    objinfo.getLineNames( linenames );
    linefld_->box()->empty();
    for  ( int idx=0; idx<linenames.size();idx++ )
	linefld_->box()->addItem( linenames.get(idx).buf() );
}

   
bool uiFingerPrintAttrib::areUIParsOK()
{
    if ( !calcobj_->getValues().size() || !calcobj_->getRanges().size() )
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

