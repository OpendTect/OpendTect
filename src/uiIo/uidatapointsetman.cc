/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$";

#include "uidatapointsetman.h"

#include "uidatapointsetmerger.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uimsg.h"

#include "bufstring.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "ptrman.h"

static const int cPrefWidth = 75;

uiDataPointSetMan::uiDataPointSetMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Manage Cross-plot Data",mNoDlgTitle,
				     "103.1.17").nrstatusflds(1),
	           PosVecDataSetTranslatorGroup::ioContext())
{
    createDefaultUI();

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    manipgrp->addButton( "mergeseis", "Merge CrossPlot",
			 mCB(this,uiDataPointSetMan,mergePush) );

    selgrp_->setPrefWidthInChar( cPrefWidth );
    infofld_->setPrefWidthInChar( cPrefWidth );
    selChg( this );
}


uiDataPointSetMan::~uiDataPointSetMan()
{
}


#define mGetDPS(dps) \
    PosVecDataSet pvds; \
    DataPointSet* dps = 0; \
    mDynamicCast(PosVecDataSetTranslator*, \
		 PtrMan<PosVecDataSetTranslator> pvdstr, \
		 curioobj_ ? curioobj_->createTranslator() : 0); \
    if ( pvdstr ) \
    { \
	pvdstr->read( *curioobj_, pvds ); \
	dps = new DataPointSet( pvds, false ); \
	dps->setName( curioobj_->name() ); \
    }


void uiDataPointSetMan::mergePush( CallBacker* )
{
    mGetDPS(dps);
    if ( !dps ) return;

    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = true;

    BufferString lbl( "Select CrossPlot data to merge with '" );
    lbl += dps->name(); lbl += "'";
    uiIOObjSelDlg seldlg( this, ctio, lbl );
    if ( !seldlg.go() )
	return;

    BufferString masternm( dps->name() );
    if ( masternm == seldlg.ioObj()->name() )
	return uiMSG().error( "Cannot merge same crossplots." );
    
    PosVecDataSet spvds;
    BufferString errmsg;
    bool rv = spvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    if ( !rv )
    { uiMSG().error( errmsg ); return; }
    if ( spvds.data().isEmpty() )
    { uiMSG().error("Selected data set is empty"); return; }

    DataPointSet* sdps =
	new DataPointSet( spvds, dps->is2D(), dps->isMinimal() );
    sdps->setName( seldlg.ioObj()->name() );

    uiDataPointSetMerger merger( this, dps, sdps );
    merger.go();
    if ( curioobj_ )
	selgrp_->fullUpdate( curioobj_->key() );
}


void uiDataPointSetMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    txt += getFileInfo();

    mGetDPS(dps);
    if ( !dps ) return;

    txt += "Properties: \n";
    for ( int colnr=0; colnr<dps->nrCols(); colnr++ )
    {
	txt += dps->colName( colnr );
	txt += " [ ";
	Interval<float> valrg =
	    dps->bivSet().valRange( dps->bivSetIdx(colnr) );
	txt += valrg.start; txt += ", ";
	txt += valrg.stop; txt += " ]";
	txt += "\n";
    }

    delete dps;
    setInfo( txt );
}
