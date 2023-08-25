/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidatapointsetio.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "uimsg.h"

#include "filepath.h"
#include "ioman.h"
#include "oddirs.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"


// uiDataPointSetSave
uiDataPointSetSave::uiDataPointSetSave( uiParent* p, const uiString& caption,
					HelpKey key,const DataPointSet* dps)
    : uiDialog(p,Setup(caption,mNoDlgTitle,mTODOHelpKey))
{
    dps_ = dps;
}


uiDataPointSetSave::~uiDataPointSetSave()
{}


bool uiDataPointSetSave::save( const char* fnm, bool ascii )
{
    if ( !dps_ )
	return false;

    BufferString errmsg;
    const bool ret = dps_->dataSet().putTo( fnm, errmsg, ascii );
    if ( !ret )
    {
	uiMSG().error( toUiString(errmsg) );
	return false;
    }

    uiMSG().message( tr("Cross-plot Data successfully saved") );
    return true;
}



#define mErrRet(s) { uiMSG().error(s); return false; }

// uiExportDataPointSet
uiExportDataPointSet::uiExportDataPointSet( uiParent* p,
					    const DataPointSet* dps )
    : uiDataPointSetSave(p,tr("Export Cross-plot Data"),
			 mODHelpKey(mDataPointSetExportHelpID),dps)
{
    if ( !dps )
    {
	IOObjContext ctxt = mIOObjContext(PosVecDataSet);
	ctxt.forread_ = true;
	infld_ = new uiIOObjSel( this, ctxt, tr("Cross-plot Data") );
	mAttachCB( infld_->selectionDone, uiExportDataPointSet::inpSelCB );
    }

    setOkText( uiStrings::sExport() );
    outfld_ = new uiASCIIFileInput( this, false );
    if ( infld_ )
	outfld_->attach( alignedBelow, infld_ );

    if ( dps )
	setOutputName( "crossplot" );
}


uiExportDataPointSet::~uiExportDataPointSet()
{
    detachAllNotifiers();
}


void uiExportDataPointSet::inpSelCB( CallBacker* )
{
    const IOObj* ioobj = infld_->ioobj( true );
    if ( !ioobj )
	return;

    const FilePath fp = ioobj->fullUserExpr();
    setOutputName( fp.baseName() );
}


void uiExportDataPointSet::setOutputName( const char* basenm )
{
    FilePath fnm( GetSurveyExportDir(), basenm );
    fnm.setExtension( "dat" );
    outfld_->setFileName( fnm.fullPath() );
}



bool uiExportDataPointSet::acceptOK( CallBacker* )
{
    if ( infld_ )
    {
	const IOObj* ioobj = infld_->ioobj();
	if ( !ioobj )
	    return false;

	PosVecDataSet pvds;
	BufferString errmsg;
	const bool rv = pvds.getFrom( ioobj->fullUserExpr(true), errmsg );
	if ( !rv )
	    mErrRet( toUiString(errmsg) );

	const bool is2d = false;
	const bool isminimal = false;
	dps_ = new DataPointSet( pvds, is2d, isminimal );
    }

    const BufferString fname = outfld_->fileName();
    if ( fname.isEmpty() )
	mErrRet( tr("Please select the output file name") )

    return save( fname.buf(), true );
}



// uiSaveCrossplotData
uiSaveCrossplotData::uiSaveCrossplotData( uiParent* p, const DataPointSet& dps,
					  const char* type )
    : uiDataPointSetSave(p,tr("Save Cross-plot Data"),
			 mODHelpKey(mDataPointSetSaveHelpID),&dps)
    , type_(type)
{
    setOkText( uiStrings::sSave() );

    IOObjContext ctxt = mIOObjContext(PosVecDataSet);
    ctxt.forread_ = false;
    if ( !type_.isEmpty() )
	ctxt.toselect_.require_.set( sKey::Type(), type_.buf() );

    outfld_ = new uiIOObjSelGrp( this, ctxt );
}


uiSaveCrossplotData::~uiSaveCrossplotData()
{}


MultiID uiSaveCrossplotData::getOutputKey() const
{
    return outfld_->currentID();
}


bool uiSaveCrossplotData::acceptOK( CallBacker* )
{
    outfld_->updateCtxtIOObj();
    const IOObj* ioobj = outfld_->getCtxtIOObj().ioobj_;
    if ( !ioobj )
	mErrRet(tr("Please enter an output name"))

    if ( !type_.isEmpty() )
    {
	ioobj->pars().set( sKey::Type(), type_ );
	IOM().commitChanges( *ioobj );
    }

    return save( ioobj->fullUserExpr(false), false );
}
