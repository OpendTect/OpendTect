/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
________________________________________________________________________

-*/

#include "uidatapointsetman.h"

#include "uidatapointsetmerger.h"
#include "uilistbox.h"
#include "uitextedit.h"
#include "uiioobjselgrp.h"
#include "uiioobjseldlg.h"
#include "uiioobjmanip.h"
#include "uimsg.h"
#include "uistrings.h"

#include "bufstring.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "ptrman.h"
#include "od_helpids.h"

static const int cPrefWidth = 75;


uiString uiDataPointSetMan::sSelDataSetEmpty()
{
    return tr("Selected data set is empty");
}


uiDataPointSetMan::uiDataPointSetMan( uiParent* p )
    : uiObjFileMan(p,
        uiDialog::Setup(uiStrings::phrManage( tr("Cross-plot Data")),
                        mNoDlgTitle,
                        mODHelpKey(mDataPointSetManHelpID) )
			       .nrstatusflds(1).modal(false),
	           PosVecDataSetTranslatorGroup::ioContext())
{
    createDefaultUI();

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    manipgrp->addButton( "mergeseis",
			 uiStrings::phrMerge(uiStrings::sCrossPlot()),
			 mCB(this,uiDataPointSetMan,mergePush) );

    selgrp_->setPrefWidthInChar( mCast(float,cPrefWidth) );
    infofld_->setPrefWidthInChar( cPrefWidth );
    selChg( this );
}


uiDataPointSetMan::~uiDataPointSetMan()
{
}


#define mGetDPS(dps,ioobj) \
    PosVecDataSet pvds; \
    RefMan<DataPointSet> dps; \
    mDynamicCast(PosVecDataSetTranslator*, \
		 PtrMan<PosVecDataSetTranslator> pvdstr, \
		 (ioobj) ? (ioobj)->createTranslator() : 0); \
    if ( pvdstr ) \
    { \
	pvdstr->read( *curioobj_, pvds ); \
	dps = new DataPointSet( pvds, false ); \
	dps->setName( curioobj_->name() ); \
    }


void uiDataPointSetMan::mergePush( CallBacker* )
{
    mGetDPS(dps,curioobj_);
    if ( !dps ) return;

    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;

    uiString lbl = tr("%1 to merge with '%2'")
	.arg(uiStrings::phrSelect(uiStrings::phrCrossPlot(uiStrings::sData())))
	.arg(toUiString(dps->name()));
    uiIOObjSelDlg seldlg( this, ctio, lbl );
    if ( !seldlg.go() )
	return;

    BufferString primarynm( dps->name() );
    if ( primarynm == seldlg.ioObj()->name() )
    {
	uiMSG().error( tr("Cannot merge same crossplots.") );
	return;
    }

    PosVecDataSet spvds;
    uiString errmsg;
    bool rv = spvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    if ( !rv )
	{ uiMSG().error( errmsg ); return; }
    if ( spvds.data().isEmpty() )
	{ uiMSG().error(sSelDataSetEmpty()); return; }

    DataPointSet* sdps =
	new DataPointSet( spvds, dps->is2D(), dps->isMinimal() );
    sdps->setName( seldlg.ioObj()->name() );

    uiDataPointSetMerger merger( this, dps, sdps );
    merger.go();
    if ( curioobj_ )
	selgrp_->fullUpdate( curioobj_->key() );
}


bool uiDataPointSetMan::gtItemInfo( const IOObj& ioobj, uiPhraseSet& inf ) const
{
    const IOObj* ioobjptr = &ioobj;
    mGetDPS(dps,ioobjptr);
    if ( !dps )
	{ inf.add( sSelDataSetEmpty() ); return false; }

    const int nrcols = dps->nrCols();
    if ( nrcols < 1 )
	return true;

    addObjInfo( inf, uiStrings::sProperty(nrcols), nrcols );
    for ( int colnr=0; colnr<nrcols; colnr++ )
    {
	const Interval<float> valrg =
	    dps->bivSet().valRange( dps->bivSetIdx(colnr) );
	BufferString kystr( "\t", dps->colName(colnr)  );
	BufferString valrgstr( "[", valrg.start );
	   valrgstr.add( " - " ).add( valrg.stop ).add( "]" );
	addObjInfo( inf, toUiString(kystr), toUiString(valrgstr) );
    }

    return true;
}
