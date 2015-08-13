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
#include "uiseistransf.h"
#include "uibatchjobdispatchersel.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "welltransl.h"
#include "wellman.h"
#include "segybatchio.h"


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
    , transffld_(0)
    , outscanfld_(0)
    , outimpfld_(0)
    , outwllfld_(0)
{
    if ( fs_.isVSP() )
	crVSPFields();
    else
	crSeisFields();

    postFinalise().notify( mCB(this,uiSEGYReadFinisher,initWin) );
}


void uiSEGYReadFinisher::crSeisFields()
{
    const Seis::GeomType gt = fs_.geomType();
    uiSeisTransfer::Setup trsu( gt );
    trsu.withnullfill( false ).fornewentry( true );
    transffld_ = new uiSeisTransfer( this, trsu );

    uiSeisSel::Setup sssu( gt ); sssu.enabotherdomain( true );
    IOObjContext ctxt( uiSeisSel::ioContext( gt, false ) );
    outimpfld_ = new uiSeisSel( this, ctxt, sssu );
    outimpfld_->attach( alignedBelow, transffld_ );

    //TODO outscanfld_ = ...

    if ( gt != Seis::Line )
    {
	batchfld_ = new uiBatchJobDispatcherSel( this, true,
						 Batch::JobSpec::SEGY );
	batchfld_->setJobName( "Read SEG-Y" );
	batchfld_->jobSpec().pars_.setYN( SEGY::IO::sKeyIs2D(), Seis::is2D(gt));
	batchfld_->attach( alignedBelow, outimpfld_ );
    }

    //TODO: Z domain handling
    //TODO field for choice import or scan
}


void uiSEGYReadFinisher::crVSPFields()
{
    outwllfld_ = new uiIOObjSel( this, mIOObjContext(Well), tr("Add to Well") );
    outwllfld_->selectionDone.notify( mCB(this,uiSEGYReadFinisher,wllSel) );
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
						    tr("Output log name") );
    lcb->attach( alignedBelow, outwllfld_ );
    lognmfld_ = lcb->box();
    lognmfld_->setReadOnly( false );
    lognmfld_->setText( "VSP" );

    //TODO: Z in file (TWT vs MD, maybe even time)
}


uiSEGYReadFinisher::~uiSEGYReadFinisher()
{
}


void uiSEGYReadFinisher::wllSel( CallBacker* )
{
    BufferStringSet nms; Well::MGR().getLogNames( outwllfld_->key(), nms );
    BufferString curlognm = lognmfld_->text();
    lognmfld_->setEmpty();
    lognmfld_->addItems( nms );
    if ( curlognm.isEmpty() )
	curlognm = "VSP";
    lognmfld_->setText( curlognm );
}


void uiSEGYReadFinisher::initWin( CallBacker* )
{
}


bool uiSEGYReadFinisher::acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: omplement actual import" );
    // TODO
    // batchfld_->jobSpec().pars_.set( SEGY::IO::sKeyTask(),
    // SEGY::IO::sKeyImport() or SEGY::IO::sKeyScan() );
    return false;
}
