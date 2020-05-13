/*+
_____________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		May 2020
________________________________________________________________________

 -*/

#include "uitoolbarcmded.h"

#include "file.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uidialog.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uistrings.h"
#include "uitoolbutton.h"

uiToolBarCommandEditor::uiToolBarCommandEditor( uiParent* p,
						const uiString& seltxt,
						const BufferStringSet& paths,
						const BufferStringSet& exenms,
						bool withcheck,
						bool mkinvisible )
: uiGroup(p)
, mkinvisible_(mkinvisible)
, checked(this)
, changed(this)
{
    uiLabeledComboBox* lblcb;
    if ( !exenms.isEmpty() )
    {
	BufferStringSet found = createUiList( paths, exenms );
	found.add( "Other" );
	lblcb = new uiLabeledComboBox( this, found, seltxt );
	lblcb->setStretch( 2, 1 );
	exeselfld_ = lblcb->box();
    }
    uiFileInput::Setup su;
    su.defseldir(paths.get(0)).forread(true);
    #ifdef __win__
    su.filter("*.exe");
    #endif
    commandfld_ = new uiFileInput( this, tr("Command"), su );
    commandfld_->setElemSzPol( uiObject::WideVar );
    if ( exeselfld_ )
	commandfld_->attach( alignedBelow, lblcb );

    argumentsfld_ = new uiGenInput( this, tr("Arguments"), StringInpSpec() );
    argumentsfld_->setElemSzPol( uiObject::WideVar );
    argumentsfld_->attach( alignedBelow, commandfld_ );

    tooltipfld_ = new uiGenInput( this, tr("Tool Tip"), StringInpSpec() );
    tooltipfld_->setElemSzPol( uiObject::WideVar );
    tooltipfld_->attach( alignedBelow, argumentsfld_ );

    iconfld_ = new uiToolButton( this, "programmer", tr("Select icon"),
				  mCB(this,uiToolBarCommandEditor,iconSelCB) );
    iconfld_->attach( rightOf, commandfld_ );

    setHAlignObj( commandfld_ );

    if ( withcheck )
    {
	checkbox_ = new uiCheckBox( this, uiString::emptyString() );
	checkbox_->setChecked( false );
	if ( exeselfld_ )
	    checkbox_->attach( leftTo, lblcb );
	else
	    checkbox_->attach( leftTo, commandfld_ );
    }
    mAttachCB(postFinalise(), uiToolBarCommandEditor::initGrp);
}


uiToolBarCommandEditor::~uiToolBarCommandEditor()
{
    detachAllNotifiers();
}


void uiToolBarCommandEditor::initGrp( CallBacker* )
{
    mAttachCB(commandfld_->valuechanged,uiToolBarCommandEditor::commandChgCB);
    mAttachCB(argumentsfld_->valuechanged,uiToolBarCommandEditor::commandChgCB);
    mAttachCB(tooltipfld_->valuechanged,uiToolBarCommandEditor::commandChgCB);

    if ( checkbox_ )
    {
	mAttachCB(checkbox_->activated, uiToolBarCommandEditor::checkCB);
	checkCB( nullptr );
    }
    if ( exeselfld_ )
    {
	mAttachCB(exeselfld_->selectionChanged,
		  uiToolBarCommandEditor::exeSelChgCB);
	exeSelChgCB( nullptr );
    }
}


void uiToolBarCommandEditor::setChecked( bool yn )
{
    if ( checkbox_ )
	checkbox_->setChecked( yn );
}


bool uiToolBarCommandEditor::isChecked() const
{
    if ( checkbox_ )
	return checkbox_->isChecked();
    else
	return true;
}


BufferStringSet uiToolBarCommandEditor::createUiList(
		const BufferStringSet& paths, const BufferStringSet& exenms )
{
    BufferStringSet res;
    for ( const auto exenm : exenms )
    {
	BufferString tmp = File::findExecutable( *exenm, paths );
	if ( !tmp.isEmpty() )
	    res.add( *exenm );
    }
    return res;
}

void uiToolBarCommandEditor::clear()
{
    setCommand( BufferString::empty() );
    setArguments( BufferString::empty() );
    setToolTip( BufferString::empty() );
    setIconFile( "programmer" );
}


BufferString uiToolBarCommandEditor::getCommand() const
{
    return commandfld_->fileName();
}


BufferString uiToolBarCommandEditor::getArguments() const
{
    return argumentsfld_->text();
}


BufferString uiToolBarCommandEditor::getToolTip() const
{
    return tooltipfld_->text();
}


BufferString uiToolBarCommandEditor::getIconFile() const
{
    return iconfile_;
}


void uiToolBarCommandEditor::setCommand( const BufferString& cmd )
{
    commandfld_->setFileName( cmd );
}


void uiToolBarCommandEditor::setArguments( const BufferString& args )
{
    argumentsfld_->setText( args );
}


void uiToolBarCommandEditor::setToolTip( const BufferString& tip )
{
    tooltipfld_->setText( tip );
}


void uiToolBarCommandEditor::setIconFile( const BufferString& iconfile )
{
    iconfld_->setIcon( iconfile );
    iconfile_ = iconfile;
}


void uiToolBarCommandEditor::advSetSensitive( bool yn )
{
    commandfld_->setSensitive( yn );
    argumentsfld_->setSensitive( yn );
    tooltipfld_->setSensitive( yn );
    iconfld_->setSensitive( yn );
}


void uiToolBarCommandEditor::advDisplay( bool yn )
{
    commandfld_->display( yn );
    argumentsfld_->display( yn );
    tooltipfld_->display( yn );
    iconfld_->display( yn );
}


void uiToolBarCommandEditor::checkCB( CallBacker* )
{
    if ( !checkbox_ )
	return;
    const bool ischecked = checkbox_->isChecked();
    if ( mkinvisible_ )
    {
	if ( exeselfld_ )
	    exeselfld_->display( ischecked );
	advDisplay( ischecked );
    }
    else
    {
	if ( exeselfld_ )
	    exeselfld_->setSensitive( ischecked );
	advSetSensitive( ischecked );
    }

    checked.trigger();
}


void uiToolBarCommandEditor::commandChgCB( CallBacker* )
{
    changed.trigger();
}


void uiToolBarCommandEditor::exeSelChgCB( CallBacker* )
{
    BufferString cmd( exeselfld_->text() );
    clear();
    if ( cmd=="Other" && isChecked() )
    {
	advSetSensitive( true );
	advDisplay( true );
    }
    else if ( isChecked() )
    {
	advSetSensitive( false );
	advDisplay( !mkinvisible_ );
	setCommand( cmd );
	setToolTip( cmd );
	setIconFile( cmd );
	commandChgCB( nullptr );
    }
}


void uiToolBarCommandEditor::iconSelCB( CallBacker* )
{
    uiFileDialog dlg( this, OD::SelectFileForRead, iconfile_, "*.png" );
    if ( ! dlg.go() )
	return;
    iconfile_ = dlg.fileName();
    iconfld_->setIcon( iconfile_ );

    commandChgCB( nullptr );
}


void uiToolBarCommandEditor::fillPar( IOPar& par ) const
{
    BufferString cmd;
    if ( exeselfld_ )
	cmd = exeselfld_->text();

    if ( isChecked() && cmd=="Other" )
    {
	par.set( sKey::Command(), getCommand() );
	par.set( sKey::Arguments(), getArguments() );
	par.set( sKey::ToolTip(), getToolTip() );
	par.set( sKey::IconFile(), getIconFile() );
    }
    else if ( isChecked() && !cmd.isEmpty() )
	par.set( sKey::ExeName(), cmd );
}


void uiToolBarCommandEditor::usePar( const IOPar& par )
{
    BufferString exenm, cmd, args, tip, iconfile;
    if ( par.get( sKey::ExeName(), exenm ) && !exenm.isEmpty()
	 && exeselfld_ )
    {
	exeselfld_->setCurrentItem( exenm );
	setChecked( true );
	exeSelChgCB( nullptr );
    }
    else if ( par.get( sKey::Command(), cmd ) && !cmd.isEmpty() )
    {
	par.get( sKey::Arguments(), args );
	par.get( sKey::ToolTip(), tip );
	par.get( sKey::IconFile(), iconfile );
	if ( exeselfld_ )
	    exeselfld_->setCurrentItem( "Other" );
	setCommand( cmd );
	setArguments( args );
	setToolTip( tip );
	setIconFile( iconfile );
	setChecked( true );
    }
    else
	setChecked( false );
}
