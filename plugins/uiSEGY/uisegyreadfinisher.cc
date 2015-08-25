/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: $";

#include "uisegyreadfinisher.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseistransf.h"
#include "uibatchjobdispatchersel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uibutton.h"
#include "uimsg.h"
#include "welltransl.h"
#include "wellman.h"
#include "segybatchio.h"
#include "segydirecttr.h"
#include "ioobj.h"
#include "filepath.h"


uiString uiSEGYReadFinisher::getWinTile( const FullSpec& fs )
{
    const Seis::GeomType gt = fs.geomType();
    const bool isvsp = fs.isVSP();

    uiString ret;
    if ( fs.spec_.nrFiles() > 1 && !isvsp && Seis::is2D(gt) )
	ret = tr("Import %1s");
    else
	ret = tr("Import %1");

    if ( isvsp )
	ret.arg( tr("Zero-offset VSP") );
    else
	ret.arg( Seis::nameOf(gt) );
    return ret;
}


uiString uiSEGYReadFinisher::getDlgTitle( const char* usrspec )
{
    uiString ret( "Importing %1" );
    ret.arg( usrspec );
    return ret;
}


uiSEGYReadFinisher::uiSEGYReadFinisher( uiParent* p, const FullSpec& fs,
					const char* usrspec )
    : uiDialog(p,uiDialog::Setup(getWinTile(fs),getDlgTitle(usrspec),
				  mTODOHelpKey ) )
    , fs_(fs)
    , outwllfld_(0)
    , lognmfld_(0)
    , inpdomfld_(0)
    , isfeetfld_(0)
    , outimpfld_(0)
    , outscanfld_(0)
    , transffld_(0)
    , batchfld_(0)
    , docopyfld_(0)
{
    objname_ = FilePath( usrspec ).baseName();
    const bool is2d = Seis::is2D( fs_.geomType() );
    if ( !is2d )
	objname_.replace( '*', 'x' );

    if ( fs_.isVSP() )
	crVSPFields();
    else
	crSeisFields();

    postFinalise().notify( mCB(this,uiSEGYReadFinisher,initWin) );
}


void uiSEGYReadFinisher::crSeisFields()
{
    const Seis::GeomType gt = fs_.geomType();
    const bool is2d = Seis::is2D( gt );

    if ( gt != Seis::Line )
    {
	docopyfld_ = new uiGenInput( this, "Copy data",
		BoolInpSpec(true,tr("Yes (import)"),tr("No (scan&&link)")) );
	docopyfld_->valuechanged.notify(mCB(this,uiSEGYReadFinisher,doScanChg));
    }

    uiSeisTransfer::Setup trsu( gt );
    trsu.withnullfill( false ).fornewentry( true );
    transffld_ = new uiSeisTransfer( this, trsu );
    if ( docopyfld_ )
	transffld_->attach( alignedBelow, docopyfld_ );
    if ( is2d )
	transffld_->selFld2D()->setSelectedLine( objname_ );

    uiSeisSel::Setup copysu( gt ); copysu.enabotherdomain( true );
    IOObjContext ctxt( uiSeisSel::ioContext( gt, false ) );
    outimpfld_ = new uiSeisSel( this, ctxt, copysu );
    outimpfld_->attach( alignedBelow, transffld_ );
    if ( !is2d )
	outimpfld_->setInputText( objname_ );

    uiSeisSel::Setup scansu( gt );
    scansu.enabotherdomain( true ).withwriteopts( false );
    ctxt.toselect.allownonuserselectable_ = true;
    ctxt.fixTranslator( SEGYDirectSeisTrcTranslator::translKey() );
    outscanfld_ = new uiSeisSel( this, ctxt, scansu );
    outscanfld_->attach( alignedBelow, transffld_ );
    if ( !is2d )
	outscanfld_->setInputText( objname_ );

    // uiGroup* lastgrp = outimpfld_;
    if ( gt != Seis::Line )
    {
	batchfld_ = new uiBatchJobDispatcherSel( this, true,
						 Batch::JobSpec::SEGY );
	batchfld_->setJobName( "Read SEG-Y" );
	batchfld_->jobSpec().pars_.setYN( SEGY::IO::sKeyIs2D(), Seis::is2D(gt));
	batchfld_->attach( alignedBelow, outimpfld_ );
	// lastgrp = batchfld_;
    }

    //TODO: Z domain handling
}


void uiSEGYReadFinisher::crVSPFields()
{
    uiString inptxt( tr("Input Z (%1-%2) is") );
    const float startz = fs_.readopts_.timeshift_;
    const float endz = startz + fs_.readopts_.sampleintv_ * (fs_.pars_.ns_-1);
    inptxt.arg( startz ).arg( endz );
    const char* doms[] = { "TWT", "TVDSS", "MD", 0 };
    inpdomfld_ = new uiGenInput( this, inptxt, StringListInpSpec(doms) );
    inpdomfld_->valuechanged.notify( mCB(this,uiSEGYReadFinisher,inpDomChg) );
    isfeetfld_ = new uiCheckBox( this, "in Feet" );
    isfeetfld_->attach( rightOf, inpdomfld_ );
    isfeetfld_->setChecked( fs_.zinfeet_ );

    outwllfld_ = new uiIOObjSel( this, mIOObjContext(Well), tr("Add to Well") );
    outwllfld_->selectionDone.notify( mCB(this,uiSEGYReadFinisher,wllSel) );
    outwllfld_->attach( alignedBelow, inpdomfld_ );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
						    tr("Output log name") );
    lcb->attach( alignedBelow, outwllfld_ );
    lognmfld_ = lcb->box();
    lognmfld_->setReadOnly( false );
    lognmfld_->setText( objname_ );
}


uiSEGYReadFinisher::~uiSEGYReadFinisher()
{
}


void uiSEGYReadFinisher::initWin( CallBacker* )
{
    inpDomChg( 0 );
    doScanChg( 0 );
}


void uiSEGYReadFinisher::wllSel( CallBacker* )
{
    if ( !lognmfld_ )
	return;

    BufferStringSet nms; Well::MGR().getLogNames( outwllfld_->key(), nms );
    BufferString curlognm = lognmfld_->text();
    lognmfld_->setEmpty();
    lognmfld_->addItems( nms );
    if ( curlognm.isEmpty() )
	curlognm = "VSP";
    lognmfld_->setText( curlognm );
}


void uiSEGYReadFinisher::inpDomChg( CallBacker* )
{
    if ( !isfeetfld_ )
	return;

    isfeetfld_->display( inpdomfld_->getIntValue() != 0 );
}


void uiSEGYReadFinisher::doScanChg( CallBacker* )
{
    if ( !docopyfld_ )
	return;

    const bool copy = docopyfld_->getBoolValue();
    outimpfld_->display( copy );
    transffld_->display( copy );
    outscanfld_->display( !copy );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSEGYReadFinisher::doVSP()
{
    const IOObj* ioobj = outwllfld_->ioobj();
    if ( !ioobj )
	return false;
    const BufferString lognm( lognmfld_->text() );
    if ( lognm.isEmpty() )
	mErrRet(tr("Please enter a valid name for the new log"))

    mErrRet( "TODO: VSP import" );
}


bool uiSEGYReadFinisher::do3D( bool doimp )
{
    uiMSG().error( "TODO: 3D non-batch import" );
    return false;
}


bool uiSEGYReadFinisher::do2D( bool doimp )
{
    uiMSG().error( "TODO: 2D non-batch import" );
    return false;
}


bool uiSEGYReadFinisher::acceptOK( CallBacker* )
{
    if ( fs_.isVSP() )
	return doVSP();

    const bool doimp = docopyfld_ ? docopyfld_->getBoolValue() : true;
    const bool dobatch = batchfld_ && batchfld_->wantBatch();
    const bool is2d = Seis::is2D( fs_.geomType() );

    if ( !dobatch )
	return is2d ? do2D( doimp ) : do3D( doimp );

    const IOObj* outioobj = outFld(doimp)->ioobj();
    if ( !outioobj )
	return false;

    const bool isps = Seis::isPS( fs_.geomType() );
    batchfld_->setJobName( doimp ? "import SEG-Y" : "scan SEG-Y" );

    IOPar& jobpars = batchfld_->jobSpec().pars_;
    jobpars.set( SEGY::IO::sKeyTask(), doimp ? SEGY::IO::sKeyImport()
	    : (isps ? SEGY::IO::sKeyIndexPS() : SEGY::IO::sKeyIndex3DVol()) );
    fs_.spec_.fillPar( jobpars );
    fs_.pars_.fillPar( jobpars );
    fs_.readopts_.fillPar( jobpars );

    IOPar outpars;
    transffld_->fillPar( outpars );
    outFld(doimp)->fillPar( outpars );
    jobpars.mergeComp( outpars, sKey::Output() );

    return batchfld_->start();

}
