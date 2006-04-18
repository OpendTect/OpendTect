/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Payraudeau
 Date:          February  2006
 RCS:           $Id: uifingerprintattrib.cc,v 1.1 2006-04-18 11:09:05 cvshelene Exp $

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
    table_ = new uiTable( this, uiTable::Setup().rowdesc("")
						.rowgrow(true)
						.defrowlbl("")
						.fillcol(true)
						.fillrow(false) );

    const char* collbls[] = { "Reference attributes", "Values", 0 };
    table_->setColumnLabels( collbls );
    table_->setNrRows( sInitNrRows );
    table_->setColumnWidth(0,240);
    table_->setColumnWidth(1,80);
    table_->setRowHeight( -1, 20 );
    table_->setStretch( 0, 0 );
    table_->setToolTip( "Right-click to add, insert or remove an attribute" );
    table_->setPrefHeight( 225 );

    const char* butlbl = "&Obtain values from a reference position";
    getvalbut_ = new uiPushButton( this, butlbl, false );
    getvalbut_->activated.notify( mCB(this,uiFingerPrintAttrib,getValPush) );
    getvalbut_->attach( alignedBelow, table_ );
    
    getvaldlg_ = new uiFingerPrintGetValDlg(p);
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


void uiFingerPrintAttrib::getValPush( CallBacker* cb )
{
    getvaldlg_->windowClosed.notify(mCB(this,uiFingerPrintAttrib,valDlgClosed));
    getvaldlg_->setDescSet( ads );
    getvaldlg_->go();
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
    getvaldlg_->set2D( yn );
}


bool uiFingerPrintAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),FingerPrint::attribName()) )
	return false;

    mIfGetBinID( FingerPrint::refposStr(), refpos, 
	    	 getvaldlg_->setRefBinID(refpos) )
    mIfGetFloat( FingerPrint::refposzStr(), refposz,
	    	 getvaldlg_->setRefZ( refposz*SI().zFactor() ) );
    
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

    mSetBinID( FingerPrint::refposStr(), getvaldlg_->getRefBinID() );

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


void uiFingerPrintAttrib::valDlgClosed( CallBacker* )
{
    if ( getvaldlg_->uiResult() == 0 )
	return;

    TypeSet<float> vals = getvaldlg_->getValues();
    for ( int idx=0; idx<attribflds_.size(); idx++ )
    {
	BufferString inp = attribflds_[idx]->getInput();
	for ( int idxdesc=0; idxdesc<ads->nrDescs(); idxdesc++ )
	{
	    if ( !strcmp( inp, ads->desc(idxdesc)->userRef() ) )
		table_->setValue( RowCol(idx,1), vals[idxdesc] );
	}
    }
}

//------------------------------------------------------------------------------
uiFingerPrintGetValDlg::uiFingerPrintGetValDlg( uiParent* p )
        : uiDialog( p, uiDialog::Setup("FingerPrint dialog","").modal(false) )
{
    BufferString title( "Get values from a reference position" );
    setTitleText( title );
    
    uiGroup* topgrp = new uiGroup( this, "position group" );
    refposfld_ = new uiStepOutSel( topgrp, "Position" );
    refposzfld_ = new uiGenInput( topgrp, "Z" );
    refposzfld_->setElemSzPol( uiObject::Small );
    refposzfld_->attach( rightOf, refposfld_ );

    CallBack cb = mCB(this,uiFingerPrintGetValDlg,getPosPush);
    const ioPixmap pm( GetIconFileName("pick.png") );
    getposbut_ = new uiToolButton( topgrp, "", pm, cb );
    getposbut_->attach( rightOf, refposzfld_ );
    
    CallBack cbcalc = mCB(this,uiFingerPrintGetValDlg,calcPush);
    calcbut_ = new uiPushButton( this, "Get &values", cbcalc, true );
    calcbut_->attach( centeredBelow, topgrp );
}


void uiFingerPrintGetValDlg::set2D( bool yn )
{
    refposfld_->set2D( yn );
}


void uiFingerPrintGetValDlg::getPosPush(CallBacker*)
{
}


void uiFingerPrintGetValDlg::calcPush(CallBacker*)
{
    refpos_ = refposfld_->binID();
    refposz_ = refposzfld_->getfValue() / SI().zFactor();

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

    saveValues( values[0] );
    
}


EngineMan* uiFingerPrintGetValDlg::createEngineMan()
{
    EngineMan* aem = new EngineMan;
    
    TypeSet<SelSpec> attribspecs;
    for ( int idx=0; idx<attrset_->nrDescs(); idx++ )
    {
	SelSpec sp( 0, attrset_->desc(idx)->id() );
	attribspecs += sp;
    }

    aem->setAttribSet( attrset_ );
    aem->setAttribSpecs( attribspecs );

    return aem;
    
}


void uiFingerPrintGetValDlg::saveValues( BinIDValueSet* bidvalset )
{
    float* vals = bidvalset->getVals( bidvalset->getPos(0) );
    for ( int idx=1; idx<bidvalset->nrVals(); idx++ )
	values_ += vals[idx];
}

