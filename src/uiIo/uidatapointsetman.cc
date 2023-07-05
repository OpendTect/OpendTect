/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidatapointsetman.h"

#include "bufstring.h"
#include "datapointset.h"
#include "od_helpids.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "ptrman.h"

#include "uidatapointsetmerger.h"
#include "uiioobjmanip.h"
#include "uiioobjseldlg.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitextedit.h"

static const int cPrefWidth = 75;


uiString uiDataPointSetMan::sSelDataSetEmpty()
{ return tr("Selected data set is empty"); }


uiDataPointSetMan::uiDataPointSetMan( uiParent* p )
    : uiObjFileMan(p,
	uiDialog::Setup(uiStrings::phrManage( tr("Cross-plot Data")),
			mNoDlgTitle,mODHelpKey(mDataPointSetManHelpID))
				.nrstatusflds(1).modal(false),
	PosVecDataSetTranslatorGroup::ioContext())
{
    createDefaultUI();

    addManipButton( "mergeseis", uiStrings::phrMerge(uiStrings::sCrossPlot()),
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
    RefMan<DataPointSet> dps = 0; \
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
    if ( !dps )
	return;

    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;

    uiString lbl = tr("%1 to merge with '%2'")
	.arg(uiStrings::phrSelect(uiStrings::phrCrossPlot(uiStrings::sData())))
	.arg(toUiString(dps->name()));
    uiIOObjSelDlg seldlg( this, ctio, lbl );
    if ( !seldlg.go() )
	return;

    const BufferString primarynm( dps->name() );
    if ( primarynm.isEqual(seldlg.ioObj()->name()) )
	return uiMSG().error( tr("Cannot merge same crossplots.") );

    PosVecDataSet spvds;
    BufferString errmsg;
    bool rv = spvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    if ( !rv )
    {
	uiMSG().error( mToUiStringTodo(errmsg) );
	return;
    }

    if ( spvds.data().isEmpty() )
    {
	uiMSG().error(sSelDataSetEmpty());
	return;
    }

    RefMan<DataPointSet> sdps =
	new DataPointSet( spvds, dps->is2D(), dps->isMinimal() );
    sdps->setName( seldlg.ioObj()->name() );

    uiDataPointSetMerger merger( this, dps, sdps );
    merger.go();
    if ( curioobj_ )
	selgrp_->fullUpdate( curioobj_->key() );
}


void uiDataPointSetMan::mkFileInfo()
{
    if ( !curioobj_ )
    {
	setInfo( "" );
	return;
    }

    BufferString txt;
    txt += getFileInfo();

    mGetDPS(dps);
    if ( !dps )
	return;

    txt += "\nProperties: \n";
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

    setInfo( txt );
}
