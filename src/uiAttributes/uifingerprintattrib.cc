/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February  2006
 RCS:           $Id: uifingerprintattrib.cc,v 1.2 2006-04-21 08:13:22 cvshelene Exp $

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
#include "uitable.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "pixmap.h"
#include "oddirs.h"
#include "binidvalset.h"
#include "survinfo.h"

using namespace Attrib;

static const int sInitNrRows = 5;

uiFingerPrintAttrib::uiFingerPrintAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    usereffld_ = new uiGenInput( this, "", 
	    			 BoolInpSpec("Reference position","Manual") );
    usereffld_->valuechanged.notify( mCB(this,uiFingerPrintAttrib,isRefSel) );
    
    uiGroup* topgrp = new uiGroup( this, "position group" );
    refposfld_ = new uiStepOutSel( topgrp, "Position" );
    refposzfld_ = new uiGenInput( topgrp, "Z" );
    refposzfld_->setElemSzPol( uiObject::Small );
    refposzfld_->attach( rightOf, refposfld_ );
    
    CallBack cb = mCB(this,uiFingerPrintAttrib,getPosPush);
    const ioPixmap pm( GetIconFileName("pick.png") );
    getposbut_ = new uiToolButton( topgrp, "", pm, cb );
    getposbut_->attach( rightOf, refposzfld_ );
    topgrp->attach( centeredBelow, usereffld_ );

    table_ = new uiTable( this, uiTable::Setup().rowdesc("")
						.rowgrow(true)
						.defrowlbl("")
						.fillcol(true)
						.fillrow(false) );

    const char* collbls[] = { "Reference attributes", "Values", 0 };
    table_->setColumnLabels( collbls );
    table_->setNrRows( sInitNrRows );
    table_->setColumnWidth(0,240);
    table_->setColumnWidth(1,90);
    table_->setRowHeight( -1, 16 );
    table_->setToolTip( "Right-click to add, insert or remove an attribute" );
    table_->attach( centeredBelow, topgrp );

    uiLabel* calclbl = 
	new uiLabel( this, "Attribute values at reference position " );
    calclbl->attach( alignedBelow, table_ );
    CallBack cbcalc = mCB(this,uiFingerPrintAttrib,calcPush);
    calcbut_ = new uiPushButton( this, "C&alculate", cbcalc, true );
    calcbut_->attach( rightTo, calclbl );

    isRefSel(0);
}


void uiFingerPrintAttrib::initTable()
{
    attribflds_.erase();
    for ( int idx=0; idx<sInitNrRows; idx++ )
    {
	uiAttrSel* attrbox = new uiAttrSel( 0, 0, "" );
	attribflds_ += attrbox;
	table_->setCellObject( RowCol(idx,0), attrbox->attachObj() );
    }

    table_->rowInserted.notify( mCB(this,uiFingerPrintAttrib,insertRowCB) );
    table_->rowDeleted.notify( mCB(this,uiFingerPrintAttrib,deleteRowCB) );
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
    int deletedrowidx = table_->currentRow();
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

    usereffld_->setValue( !mIsUdf( refposzfld_->getfValue() ) );
    isRefSel(0);
    
    table_->clearTable();
    table_->setNrRows( sInitNrRows );
    initTable();

    int nrvals = 0;
    if ( desc.getParam( FingerPrint::valStr() ) )
    {
	mDescGetConstParamGroup(FloatParam,valueset,desc,FingerPrint::valStr())
	nrvals = valueset->size();
    }

    while ( nrvals > table_->nrRows() )
	table_->insertRows(0);

    if ( desc.getParam( FingerPrint::valStr() ) )
    {
	mDescGetConstParamGroup(FloatParam,valueset,desc,FingerPrint::valStr());
	for ( int idx=0; idx<valueset->size(); idx++ )
	{
	    const ValParam& param = (ValParam&)(*valueset)[idx];
	    table_->setValue( RowCol(idx,1), param.getfValue(0) );
	}
    }

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

    mSetBinID( FingerPrint::refposStr(), refposfld_->binID() );
    mSetFloat( FingerPrint::refposzStr(), refposzfld_->getfValue() );

    TypeSet<float> values;
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	float val = table_->getfValue( RowCol(idx,1) );
	if ( mIsUdf(val) ) continue;

	values += val;
    }

    mDescGetParamGroup(FloatParam,valueset,desc,FingerPrint::valStr())
    valueset->setSize( values.size() );
    for ( int idx=0; idx<values.size(); idx++ )
    {
	ValParam& valparam = (ValParam&)(*valueset)[idx];
	valparam.setValue(values[idx] );
    }
    
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


void uiFingerPrintAttrib::isRefSel( CallBacker* )
{
    const bool useref = usereffld_-> getBoolValue();
    refposfld_->display( useref );
    refposzfld_->display( useref );
    getposbut_->display( useref );
}


void uiFingerPrintAttrib::getPosPush(CallBacker*)
{
}


void uiFingerPrintAttrib::calcPush(CallBacker*)
{
    BinID refpos_ = refposfld_->binID();
    float refposz_ = refposzfld_->getfValue() / SI().zFactor();

    if ( mIsUdf( refpos_.inl ) || mIsUdf( refpos_.crl ) || mIsUdf( refposz_ ) )
    {
	//see if ok in 2d
	uiMSG().error( "Please fill in the position fields first" );
	return;
    }

    ObjectSet<BinIDValueSet> values;
    BinIDValueSet bidvalset( 1, false );
    bidvalset.add( refpos_, refposz_ );
    values += &bidvalset;
    
    BufferString errmsg;
    PtrMan<Attrib::EngineMan> aem = createEngineMan();
    PtrMan<Processor> proc = aem->createLocationOutput( errmsg, values );
    if ( !proc )
	uiMSG().error(errmsg);

    int res = proc->nextStep();
    if ( res == -1 )
    {
	uiMSG().error( "Cannot reach next position" );
	return;
    }

    showValues( values[0] );
    
}


EngineMan* uiFingerPrintAttrib::createEngineMan()
{
    EngineMan* aem = new EngineMan;
    
    TypeSet<SelSpec> attribspecs;
    for ( int idx=0; idx<ads->nrDescs(); idx++ )
    {
	SelSpec sp( 0, ads->desc(idx)->id() );
	attribspecs += sp;
    }

    aem->setAttribSet( ads );
    aem->setAttribSpecs( attribspecs );

    return aem;
    
}


void uiFingerPrintAttrib::showValues( BinIDValueSet* bidvalset )
{
    float* vals = bidvalset->getVals( bidvalset->getPos(0) );
    
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	BufferString inp = attribflds_[idx]->getInput();
	for ( int idxdesc=0; idxdesc<ads->nrDescs(); idxdesc++ )
	{
	    if ( !strcmp( inp, ads->desc(idxdesc)->userRef() ) )
		table_->setValue( RowCol(idx,1), vals[idxdesc+1] );
	}
    }
}



