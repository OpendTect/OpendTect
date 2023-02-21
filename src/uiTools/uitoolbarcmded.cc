/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolbarcmded.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uitoolbutton.h"

#include "commanddefs.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"



uiToolBarCommandEditor::uiToolBarCommandEditor( uiParent* p,
						const uiString& seltxt,
						const CommandDefs& commands,
						bool withcheck,
						bool mkinvisible )
    : uiGroup(p)
    , commands_(*new CommandDefs(commands))
    , mkinvisible_(mkinvisible)
    , checked(this)
    , changed(this)
{
    initui( seltxt, BufferStringSet(), withcheck );
}

uiToolBarCommandEditor::uiToolBarCommandEditor( uiParent* p,
						const uiString& seltxt,
						const BufferStringSet& paths,
						const BufferStringSet& exenms,
						bool withcheck,
						bool mkinvisible )
    : uiGroup(p)
    , commands_(*new CommandDefs)
    , mkinvisible_(mkinvisible)
    , checked(this)
    , changed(this)
{
    for ( auto* exenm : exenms )
	commands_.addCmd( *exenm, toUiString( *exenm ), *exenm,
			  toUiString(*exenm), paths );

    initui( seltxt, paths, withcheck );
}


void uiToolBarCommandEditor::initui( const uiString& seltxt,
				     const BufferStringSet& paths,
				     bool withcheck )
{
    uiLabeledComboBox* lblcb = nullptr;
    if ( !commands_.isEmpty() )
    {
	uiStringSet uinames( commands_.getUiNames() );
	uinames.add( uiStrings::sOther() );
	lblcb = new uiLabeledComboBox( this, uinames, seltxt );
	lblcb->setStretch( 2, 1 );
	exeselfld_ = lblcb->box();
    }

    uiFileInput::Setup su;
    su.defseldir( paths.isEmpty() ? GetSoftwareDir(true) :
				    (const char*) paths.get(0) ).forread(true);
#ifdef __win__
    su.filter("*.exe");
#endif
    commandfld_ = new uiFileInput( this, uiStrings::sCommand(), su );
    commandfld_->setElemSzPol( uiObject::WideVar );
    if ( exeselfld_ )
	commandfld_->attach( alignedBelow, lblcb );

    tooltipfld_ = new uiGenInput( this, uiStrings::sTooltip(),
				  StringInpSpec() );
    tooltipfld_->setElemSzPol( uiObject::WideVar );
    tooltipfld_->attach( alignedBelow, commandfld_ );

    iconfld_ = new uiToolButton( this, "programmer", uiStrings::sSelectIcon(),
				 mCB(this,uiToolBarCommandEditor,iconSelCB) );
    iconfld_->attach( rightOf, commandfld_ );

    setHAlignObj( commandfld_ );

    if ( withcheck )
    {
	checkbox_ = new uiCheckBox( this, uiString::emptyString() );
	checkbox_->setChecked( false );
	exeselfld_ ? checkbox_->attach( leftTo, lblcb )
		   : checkbox_->attach( leftTo, commandfld_ );
    }

    mAttachCB(postFinalize(), uiToolBarCommandEditor::initGrp);
}


uiToolBarCommandEditor::~uiToolBarCommandEditor()
{
    detachAllNotifiers();
    delete &commands_;
}


void uiToolBarCommandEditor::initGrp( CallBacker* )
{
    mAttachCB(commandfld_->valueChanged,uiToolBarCommandEditor::commandChgCB);
    mAttachCB(tooltipfld_->valueChanged,uiToolBarCommandEditor::commandChgCB);

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
    return checkbox_ ? checkbox_->isChecked() : true;
}


void uiToolBarCommandEditor::updateCmdList( const CommandDefs& commands )
{
    commands_ = commands;
    if ( exeselfld_ )
    {
	uiStringSet uinames( commands_.getUiNames() );
	uinames.add( uiStrings::sOther() );
	NotifyStopper stopselchg( exeselfld_->selectionChanged );
	NotifyStopper stopchg( changed );
	BufferString current = exeselfld_->text();
	exeselfld_->setEmpty();
	exeselfld_->addItems( uinames );
	exeselfld_->setCurrentItem( current.buf() );
	exeSelChgCB( nullptr );
    }
}


void uiToolBarCommandEditor::clear()
{
    setCommand( BufferString::empty() );
    setToolTip( BufferString::empty() );
    setIconFile( "programmer" );
}


BufferString uiToolBarCommandEditor::getCommand() const
{
    int current = commands_.size();
    if ( exeselfld_ )
	current = exeselfld_->currentItem();

    if ( current==commands_.size() )
	return commandfld_->fileName();
    else
	return commands_.get( current );
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
    tooltipfld_->setSensitive( yn );
    iconfld_->setSensitive( yn );
}


void uiToolBarCommandEditor::advDisplay( bool yn )
{
    commandfld_->display( yn );
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
	{
	    exeselfld_->display( ischecked );
	    if ( ischecked )
		exeSelChgCB( nullptr );
	}
	else
	    advDisplay( ischecked );
    }
    else
    {
	if ( exeselfld_ )
	{
	    exeselfld_->setSensitive( ischecked );
	    exeSelChgCB( nullptr );
	}
	else
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
    int current = commands_.size();
    if ( exeselfld_  )
	current = exeselfld_->currentItem();
    if ( isChecked() && current==commands_.size() )
    {
	advSetSensitive( true );
	advDisplay( true );
    }
    else if ( isChecked() && !commands_.isEmpty() )
    {
	advSetSensitive( false );
	advDisplay( !mkinvisible_ );
	setCommand( commands_.get( current ) );
	setToolTip( commands_.getToolTip( current ).getString() );
	setIconFile( commands_.getIconName( current ) );
	commandChgCB( nullptr );
    }
    else if ( !isChecked() )
	advSetSensitive( false );
}


void uiToolBarCommandEditor::iconSelCB( CallBacker* )
{
    const FilePath iconsdirfp(
	    GetSetupDataFileDir(ODSetupLoc_ApplSetupPref,false),
	    "icons.Default" );
    FilePath iconfp( iconfile_ );
    if ( iconsdirfp.exists() && !iconfp.isAbsolute() )
    {
	iconfp.insert( iconsdirfp.fullPath() );
	iconfp.setExtension( "png" );
    }

    const char* filter = "PNG (*.png);;JPEG (*.jpg *.jpeg);; ICO (*.ico)";
    uiFileDialog dlg( this, uiFileDialog::ExistingFile, iconfp.fullPath(),
		      filter );
    if ( ! dlg.go() )
	return;

    iconfile_ = dlg.fileName();
    iconfld_->setIcon( iconfile_ );

    commandChgCB( nullptr );
}


void uiToolBarCommandEditor::fillPar( IOPar& par ) const
{
    int current = commands_.size();
    if ( exeselfld_ )
	current = exeselfld_->currentItem();

    if ( isChecked() && current==commands_.size() )
    {
	par.set( sKey::Command(), getCommand() );
	par.set( sKey::ToolTip(), toUiString(getToolTip()) );
	par.set( sKey::IconFile(), getIconFile() );
    }
    else if ( isChecked() && !commands_.isEmpty() )
	par.set( sKey::ExeName(), commands_.get( current ) );
}


void uiToolBarCommandEditor::usePar( const IOPar& par )
{
    BufferString exenm, cmd, iconfile;
    uiString tip;
    if ( par.get( sKey::ExeName(), exenm ) && !exenm.isEmpty() && exeselfld_ )
    {
	int idx = commands_.indexOf( exenm );
	exeselfld_->setCurrentItem( idx==-1 ? 0 : idx );
	setChecked( true );
    }
    else if ( par.get( sKey::Command(), cmd ) && !cmd.isEmpty() )
    {
	par.get( sKey::ToolTip(), tip );
	par.get( sKey::IconFile(), iconfile );
	if ( exeselfld_ )
	    exeselfld_->setCurrentItem( "Other" );

	setCommand( cmd );
	setToolTip( tip.getString() );
	setIconFile( iconfile );
	setChecked( true );
    }
    else
	setChecked( false );

    exeSelChgCB( nullptr );
}
