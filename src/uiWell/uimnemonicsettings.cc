/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimnemonicsettings.h"

#include "filepath.h"
#include "mnemonics.h"
#include "oddirs.h"
#include "separstr.h"
#include "settings.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uifiledlg.h"
#include "uilistbox.h"
#include "uimnemonicprops.h"
#include "uimsg.h"
#include "uistrings.h"
#include "unitofmeasure.h"


uiMnemonicSettings::uiMnemonicSettings( uiParent* p, Settings& setts )
    : uiSettingsGroup(p, uiStrings::sMnemonic(), setts)
{
    auto* lb = new uiLabeledListBox( this, uiStrings::sMnemonic(),
			       OD::ChooseOnlyOne, uiLabeledListBox::AboveLeft );
    mnemonicsfld_ = lb->box();

    auto* rightgrp = new uiGroup( this );
    mnempropsfld_ = new uiMnemonicProperties( rightgrp );

    auto* butgrp = new uiButtonGroup( rightgrp, "", OD::Horizontal );
    butgrp->attach( rightAlignedBelow, mnempropsfld_ );
    applybut_ = new uiPushButton( butgrp, uiStrings::sApply(), "apply",
				  mCB(this,uiMnemonicSettings,applyCB), true );
    removebut_ = new uiPushButton( butgrp, uiStrings::sRemove(), "remove",
			       mCB(this,uiMnemonicSettings,removeCB), true );
    importbut_ = new uiPushButton( butgrp, uiStrings::sImport(), "import",
				  mCB(this,uiMnemonicSettings,importCB), true );
    exportbut_ = new uiPushButton( butgrp, uiStrings::sExport(), "export",
				  mCB(this,uiMnemonicSettings,exportCB), true );

    rightgrp->attach( centeredRightOf, mnemonicsfld_ );

    mAttachCB(mnemonicsfld_->selectionChanged, uiMnemonicSettings::mnemSelCB);
    mAttachCB(mnempropsfld_->valueChanged, uiMnemonicSettings::updateCB);

    mnemsetts_ = Mnemonic::getUserMnemonics();
    fillMnemonicList();
    mnemSelCB( nullptr );
}


uiMnemonicSettings::~uiMnemonicSettings()
{
    detachAllNotifiers();
}


bool uiMnemonicSettings::acceptOK()
{
    if ( changed_ )
	Mnemonic::setUserMnemonics( mnemsetts_ );

    return true;
}


HelpKey uiMnemonicSettings::helpKey() const
{
    return mNoHelpKey;
}


void uiMnemonicSettings::fillMnemonicList()
{
    BufferStringSet mnemnms;
    MNC().getNames( mnemnms );
    mnemnms.sort();
    mnemonicsfld_->setEmpty();
    mnemonicsfld_->addItems( mnemnms );

    IOParIterator iter( mnemsetts_ );
    BufferString key, val;
    while ( iter.next(key,val) )
    {
	if ( val.isEmpty() )
	    continue;

	int idx = mnemonicsfld_->indexOf( key );
	if ( idx>=0 )
	    mnemonicsfld_->setMarked( idx, uiListBox::Pixmap );
    }
}


void uiMnemonicSettings::mnemSelCB( CallBacker* )
{
    NotifyStopper ns1( mnempropsfld_->valueChanged );
    BufferString mnsel( mnemonicsfld_->getText() );
    const Mnemonic* mn = MNC().getByName( mnsel, false );
    if ( !mn )
	return;

    Mnemonic::Scale scale;
    OD::LineStyle lstyle;
    Interval<float> range;
    BufferString unitlbl;
    const UnitOfMeasure* uom = mn->getDisplayInfo( mnemsetts_, scale, range,
						   unitlbl, lstyle );
    mnempropsfld_->setScale( scale );
    mnempropsfld_->setRange( range );
    mnempropsfld_->setLineStyle( lstyle );
    mnempropsfld_->setUOM( uom );

    const bool hasoverride = mnemsetts_.isPresent( mnsel );
    applybut_->setSensitive( hasoverride );
    removebut_->setSensitive( hasoverride );
}


void uiMnemonicSettings::updateCB( CallBacker* )
{
    applybut_->setSensitive( true );
}


void uiMnemonicSettings::applyCB( CallBacker* )
{
    const int idx = mnemonicsfld_->currentItem();
    mnemonicsfld_->setMarked( idx, uiListBox::Pixmap );
    const BufferString mnsel( mnemonicsfld_->getText() );
    mnemsetts_.set( mnsel, mnempropsfld_->toString() );
    changed_ = true;
    needsrenewal_ = true;
    mnemSelCB( nullptr );
}


void uiMnemonicSettings::removeCB( CallBacker* )
{
    const int idx = mnemonicsfld_->currentItem();
    mnemonicsfld_->setMarked( idx, uiListBox::None );
    const BufferString mnsel( mnemonicsfld_->getText() );
    mnemsetts_.removeWithKey( mnsel );
    changed_ = true;
    needsrenewal_ = true;
    mnemSelCB( nullptr );
}


void uiMnemonicSettings::importCB( CallBacker* )
{
    uiFileDialog fdlg( this, true, nullptr, nullptr,
		       tr("Import Mnemonic Overrides") );
    if ( fdlg.go() )
    {
	IOPar par;
	par.read( fdlg.fileName(), nullptr );
	mnemsetts_.merge( par );
	BufferString cursel = mnemonicsfld_->getText();
	fillMnemonicList();
	mnemonicsfld_->setCurrentItem( cursel.buf() );
	changed_ = true;
	needsrenewal_ = true;
	mnemSelCB( nullptr );
    }
}


void uiMnemonicSettings::exportCB( CallBacker* )
{
    if ( mnemsetts_.isEmpty() )
    {
	uiMSG().message(tr( "Nothing to export.") );
	return;
    }

    uiFileDialog fdlg( this, false, nullptr, nullptr,
		       tr("Export Mnemonic Overrides") );
    if ( fdlg.go() )
	mnemsetts_.write( fdlg.fileName(), nullptr );
}
