/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Khushnood Qadir
 Date:		May 2020
________________________________________________________________________

-*/


#include "uiwelllogmerge.h"

#include "uicombobox.h"
#include "uichecklist.h"
#include "uigeninput.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistring.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"
#include "uiwelllogtools.h"
#include "uiwelllogdisplay.h"

#include "bufstring.h"
#include "ioman.h"
#include "propertyref.h"
#include "survinfo.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogmerge.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellreader.h"
#include "welltrack.h"
#include "wellwriter.h"
#include "task.h"


#define mErrRet(s) { uiMSG().error(s); return false; }


static uiString getDlgUiTitle( const TypeSet<MultiID>& wllids )
{
    const int sz = wllids.size();
    if ( sz < 1 )
	return od_static_tr( "getDlgUiTitle","No wells selected" );

    BufferString ret( IOM().nameOf(wllids[0]), "'" );
    for ( int idx=1; idx<sz; idx++ )
	ret.add( ", '" ).add( IOM().nameOf(wllids[idx]) ).add( "'" );

    ret = getLimitedDisplayString( ret, 50, true );
    return od_static_tr("getDlgUiTitle","Merge logs from '%1").arg(ret);
}


uiWellLogMerger::uiWellLogMerger( uiParent* p,
				  const TypeSet<MultiID>& wllids,
				  const BufferStringSet* chosenlognms )
    : uiDialog(p,uiDialog::Setup(tr("Merge logs"),getDlgUiTitle(wllids),
				 mTODOHelpKey)
				.applybutton(true).applytext(tr("Merge")) )
    , wellids_( wllids )
    , havenew_(false)
{
    if ( wellids_.isEmpty() )
    {
	new uiLabel( this, tr("No wells found in database.\nPlease import"
			      " or create a well first.") );
	setCtrlStyle( CloseOnly );
	return;
    }

    setOkCancelText( uiStrings::sSave(), uiStrings::sClose() );
    if ( chosenlognms )
	chosenlognms_ = *chosenlognms;

    uiGroup* pargrp = createParGrp( !chosenlognms || chosenlognms->isEmpty() );
    uiGroup* dispgrp = createLogDispGrp();
    dispgrp->attach( rightOf, pargrp );

    mAttachCB( postFinalise(), uiWellLogMerger::finalizeCB );
}


uiGroup* uiWellLogMerger::createParGrp( bool withlogsel )
{
    uiGroup* grp = new uiGroup( this, "merge parameters" );

    uiGroup* selbuttons = nullptr;
    if ( withlogsel )
    {
	uiStringSet types;
	types.add( uiStrings::sAll() );
	const EnumDef& enums = PropertyRef::StdTypeDef();
	for ( int idx=0; idx<enums.size(); idx++ )
	    types.add( enums.getUiStringForIndex(idx) );
	logtypefld_ = new uiGenInput( grp,
		tr("Only show logs of type"), StringListInpSpec(types) );
	mAttachCB( logtypefld_->valuechanged, uiWellLogMerger::logSetCB );

	uiListBox::Setup sul( OD::ChooseAtLeastOne,
			      tr("Available logs"), uiListBox::AboveMid );
	loglistfld_ = new uiListBox( grp, sul );
	loglistfld_->setHSzPol( uiObject::Wide );
	loglistfld_->attach( ensureBelow, logtypefld_ );

	selbuttons = new uiGroup( grp, "select buttons" );
	uiLabel* sellbl = new uiLabel( selbuttons, uiStrings::sSelect() );
	CallBack cbs = mCB(this,uiWellLogMerger,selButPush);
	toselect_ = new uiToolButton( selbuttons, uiToolButton::RightArrow,
					tr("Use selected log"), cbs );
	toselect_->attach( centeredBelow, sellbl );
	toselect_->setHSzPol( uiObject::Undef );
	fromselect_ = new uiToolButton( selbuttons, uiToolButton::LeftArrow,
					tr("Do not use selected log"), cbs );
	fromselect_->attach( alignedBelow, toselect_ );
	fromselect_->setHSzPol( uiObject::Undef );
	selbuttons->setHAlignObj( toselect_ );
	selbuttons->attach( centeredRightOf, loglistfld_ );
    }

    uiListBox::Setup susl( OD::ChooseNone, tr("Selected logs"),
						uiListBox::AboveMid );
    selectedlogsfld_ = new uiListBox( grp, susl );
    if ( selbuttons )
	selectedlogsfld_->attach( centeredRightOf, selbuttons );
    else
	selectedlogsfld_->addItems( chosenlognms_ );

    movebuttons_ = new uiGroup( grp, "move buttons" );
    uiLabel* movelbl = new uiLabel( movebuttons_, tr("Change\npriority") );
    CallBack cbm = mCB(this,uiWellLogMerger,moveButPush);
    moveupward_ = new uiToolButton( movebuttons_, uiToolButton::UpArrow,
				    tr("Move Up"), cbm );
    moveupward_->attach( centeredBelow, movelbl );
    moveupward_->setHSzPol( uiObject::Undef );
    movedownward_ = new uiToolButton( movebuttons_, uiToolButton::DownArrow,
					tr("Move Down"), cbm );
    movedownward_->attach( alignedBelow, moveupward_ );
    movedownward_->setHSzPol( uiObject::Undef );
    movebuttons_->setHAlignObj( moveupward_ );
    movebuttons_->attach( centeredRightOf, selectedlogsfld_ );

    uiSeparator* sep = new uiSeparator( grp, "sep" );
    if ( withlogsel )
	sep->attach( ensureBelow, loglistfld_ );

    sep->attach( ensureBelow, selectedlogsfld_ );

    extrapolatefld_ = new uiCheckBox( grp, tr("Extrapolate") );
    extrapolatefld_->setChecked( true );
    extrapolatefld_->attach( alignedBelow, selectedlogsfld_ );
    extrapolatefld_->attach( ensureBelow, sep );

    intrapolatefld_ = new uiCheckBox( grp, tr("Intrapolate") );
    intrapolatefld_->setChecked( true );
    intrapolatefld_->attach( rightOf, extrapolatefld_ );

    overlapfld_ = new uiGenInput( grp, tr("If logs overlap"),
		StringListInpSpec( Well::LogMerger::OverlapActionDef()) );
    overlapfld_->attach( alignedBelow, extrapolatefld_ ) ;
    mAttachCB( overlapfld_->valuechanged, uiWellLogMerger::overlapCB );

    float defsr = SI().depthsInFeet() ? 0.5f : 0.15f;
    srfld_ = new uiGenInput( grp,
			     uiStrings::phrOutput( tr("sample distance")),
			     FloatInpSpec(defsr) );
    srfld_->attach( alignedBelow, overlapfld_ );

    nmfld_ = new uiGenInput( grp, tr("Name for new log") );
    nmfld_->setElemSzPol( uiObject::Wide );
    nmfld_->attach( alignedBelow, srfld_ );

    uiUnitSel::Setup uussu( PropertyRef::Other,
			    uiStrings::phrOutput(tr("unit of measure")) );
    uussu.withnone( true );
    outunfld_ = new uiUnitSel( grp, uussu );
    outunfld_->inpFld()->setHSzPol( uiObject::MedVar );
    outunfld_->attach( alignedBelow, nmfld_ );

    mAttachCB( applyPushed, uiWellLogMerger::applyCB );
    return grp;
}


uiGroup* uiWellLogMerger::createLogDispGrp()
{
    uiGroup* grp = new uiGroup( this, "merged log display" );

    uiWellLogDisplay::Setup wldsu;
    logdisp_ = new uiWellLogDisplay( grp, wldsu );
    logdisp_->setPrefWidth( 150 );
    logdisp_->setPrefHeight( 450 );

    if ( wellids_.size() > 1 )
    {
	uiGroup* butgrp = new uiGroup( grp, "change log" );
	CallBack cbm = mCB(this,uiWellLogMerger,changeLogDispButPush);
	prevlog_ = new uiToolButton( butgrp, uiToolButton::LeftArrow,
					tr("Previous log"), cbm );
	prevlog_->setHSzPol( uiObject::Undef );
	nextlog_ = new uiToolButton( butgrp, uiToolButton::RightArrow,
					tr("Next log"), cbm );
	nextlog_->attach( rightOf, prevlog_ );
	nextlog_->setHSzPol( uiObject::Undef );
	butgrp->setHAlignObj( prevlog_ );
	butgrp->attach( centeredBelow, logdisp_ );
    }

    return grp;
}


uiWellLogMerger::~uiWellLogMerger()
{
    detachAllNotifiers();
    delete logdisp_;
}


void uiWellLogMerger::finalizeCB( CallBacker* )
{
    if ( logtypefld_ )
	logSetCB( nullptr );
    else
    {
	if ( !wellids_.isEmpty() && !chosenlognms_.isEmpty() )
	{
	    const Well::Data* wd = Well::MGR().get( wellids_[0] );
	    const Well::Log* log =
		wd ? wd->logs().getLog( chosenlognms_.get(0) ) : nullptr;
	    if ( log )
		outunfld_->setPropType( log->propType() );
	}

	applyCB( nullptr );
    }

    overlapCB( nullptr );
}


void uiWellLogMerger::logSetCB( CallBacker* )
{
    loglistfld_->setEmpty();

    int logsel = logtypefld_->getIntValue();
    const bool isall = logsel==0;

    BufferStringSet lognms;
    if ( isall )
    {
	for ( int idx=0; idx<wellids_.size(); idx++ )
	{
	    BufferStringSet nms;
	    Well::MGR().getLogNames( wellids_[idx], nms, true );
	    lognms.add( nms, false );
	}

	lognms.sort();
	loglistfld_->addItems( lognms );
	return;
    }

    logsel--;
    PropertyRef::StdType logprop =
		PropertyRef::StdTypeDef().getEnumForIndex( logsel );
    outunfld_->setPropType( logprop );
    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	Well::Data* wd = Well::MGR().get( wellids_[idx] );
	const TypeSet<int> lgids = wd->logs().getSuitable( logprop );
	for ( int idl=0; idl<lgids.size(); idl++ )
	    lognms.addIfNew( wd->logs().getLog(lgids[idl]).name() );
    }
    loglistfld_->addItems( lognms );
}


void uiWellLogMerger::overlapCB( CallBacker* )
{
    const int overlapaction = overlapfld_->getIntValue();
    action_= Well::LogMerger::OverlapActionDef().getEnumForIndex(overlapaction);
    movebuttons_->display( action_ == Well::LogMerger::UseOneLog );
}


void uiWellLogMerger::selButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    if ( but == toselect_ )
    {
	BufferStringSet chosenlgnms;
	for ( int idx=0; idx<loglistfld_->size(); idx++ )
	{
	    if ( loglistfld_->isChosen(idx) )
	    {
		if ( !selectedlogsfld_->isPresent(
					    loglistfld_->textOfItem(idx)) )
		    chosenlgnms.add( loglistfld_->textOfItem(idx) );
	    }
	}
	selectedlogsfld_->addItems( chosenlgnms );
	chosenlgnms.setEmpty();
    }
    else if ( but == fromselect_ )
    {
	BufferString text = selectedlogsfld_->getText();
	selectedlogsfld_->removeItem( selectedlogsfld_->currentItem() );
	int idx = loglistfld_->indexOf( text );
	loglistfld_->setChosen( idx, false );
    }

    if ( !chosenlognms_.isEmpty() )
	chosenlognms_.setEmpty();

    selectedlogsfld_->getItems( chosenlognms_ );
}


void uiWellLogMerger::moveButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    int index = selectedlogsfld_->currentItem();
    BufferString text = selectedlogsfld_->getText();
    if ( but == moveupward_ && index>0 )
    {
	selectedlogsfld_->removeItem( index );
	selectedlogsfld_->insertItem( toUiString(text), index-1 );
	selectedlogsfld_->setCurrentItem( index-1 );
    }
    else if ( but == movedownward_ && index<selectedlogsfld_->size()-1 )
    {
	selectedlogsfld_->removeItem( index );
	selectedlogsfld_->insertItem( toUiString(text), index+1 );
	selectedlogsfld_->setCurrentItem( index+1 );
    }

    if ( !chosenlognms_.isEmpty() )
	chosenlognms_.setEmpty();

    selectedlogsfld_->getItems( chosenlognms_ );
}


void uiWellLogMerger::changeLogDispButPush( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,but,cb)
    uiWellLogDisplay::LogData* wld = &logdisp_->logData();
    int index = outlogs_.indexOf( wld->log() );
    if ( but == nextlog_ && index<outlogs_.size()-1 )
    {
	wld->setLog( outlogs_[index+1] );
	logdisp_->reDraw();
    }

    if ( but == prevlog_ && index>0 )
    {
	wld->setLog( outlogs_[index-1] );
	logdisp_->reDraw();
    }
}


void uiWellLogMerger::applyCB( CallBacker* )
{
    if ( action_ == Well::LogMerger::UseAverage  && loglistfld_ )
	loglistfld_->getChosen( chosenlognms_ );

    zsampling_ = srfld_->getFValue();
    if ( mIsUdf(zsampling_) )
    {
	uiMSG().message(
		tr("Please provide the Z sample rate for the  output log") );
	return;
    }

    if ( !merge() )
    {
	uiMSG().error( tr("Could not merge logs") );
	return;
    }

    convert();
    updateLogViewer();
}


bool uiWellLogMerger::acceptOK( CallBacker* )
{
    if ( !nmfld_ )
	return false;

    if ( outlogs_.isEmpty() )
    {
	uiMSG().error( tr("Logs have not been merged yet. Please press\n"
			   "Merge button first.") );
	return false;
    }

    const BufferString newnm = getOutputLogName();
    if ( newnm.isEmpty() )
    {
	uiMSG().message( tr("Please provide a name for the new log") );
	return false;
    }

    uiString msg;
    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	BufferStringSet lognms;
	Well::MGR().getLogNames( wellids_[idx], lognms );
	if ( !lognms.isPresent(newnm) )
	    continue;

	if ( msg.isEmpty() )
	    msg = tr("A log with this name already exists for well(s):");
	msg.append( IOM().nameOf(wellids_[idx]), true );
    }

    bool dowrite = true;
    if ( !msg.isEmpty() )
    {
	msg.append( "\n\nDo you want to overwrite this log?" );
	dowrite = uiMSG().askGoOn( msg, uiStrings::sOverwrite(),
					uiStrings::sCancel() );
    }

    if ( dowrite )
	write( newnm );

    return false;
}


bool uiWellLogMerger::merge()
{
    deepErase( outlogs_ );
    TaskGroup taskgrp;
    BufferString newlognm = getOutputLogName();
    if ( newlognm.isEmpty() )
	newlognm = "<New Log>";

    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	Well::Log* outlog = new Well::Log( newlognm );
	outlogs_.add( outlog );
	auto* merger =
		new Well::LogMerger( wellids_[idx], chosenlognms_, *outlog );
	merger->setSamplingDist( zsampling_ );
	merger->setOverlapAction( action_ );
	merger->setDoInterpolation( intrapolatefld_->isChecked() );
	merger->setDoExtrapolation( extrapolatefld_->isChecked() );
	taskgrp.addTask( merger );
    }

    uiTaskRunner uitr( this );
    return uitr.execute( taskgrp );
}


bool uiWellLogMerger::write( const char* lognm )
{
    int nrlogswritten = 0;
    uiStringSet msgs;
    for ( int idx=0; idx<outlogs_.size(); idx++ )
    {
	outlogs_[idx]->setName( lognm );
	Well::Data* wd = Well::MGR().get( wellids_[idx] );
	bool res = false;
	if ( wd )
	{
	    wd->logs().add( outlogs_[idx] );
	    Well::Writer wtr( wellids_[idx], *wd );
	    res = wtr.putLog( *outlogs_[idx] );
	    if ( res )
		nrlogswritten++;

	}

	msgs.add( tr("%1 %2 to %3")
	    .arg(res ? "Successfully added" : "Failed to add")
	    .arg(outlogs_[idx]->name())
	    .arg(IOM().nameOf(wellids_[idx])) );
    }

    if ( !havenew_ )
	havenew_ = nrlogswritten > 0;

    if ( msgs.size() < 6 )
	uiMSG().message( msgs.cat() );
    else
    {
	uiDialog dlg( this,
	    uiDialog::Setup(tr("Log merge results"),mNoDlgTitle,mNoHelpKey) );
	dlg.setCtrlStyle( uiDialog::CloseOnly );
	uiTextEdit* fld = new uiTextEdit( &dlg, "Text", true );
	fld->setText( msgs.cat() );
	dlg.go();
    }

    return true;
}


bool uiWellLogMerger::convert()
{
    const UnitOfMeasure* outun = outunfld_->getUnit();
    if ( !outun )
	return false;

    for ( int idx=0; idx<outlogs_.size(); idx++ )
    {
	auto* log = outlogs_[idx];
	const UnitOfMeasure* logun = log->unitOfMeasure();
	for ( int idl=0; idl<log->size(); idl++  )
	{
	    const float initialval = log->value( idl );
	    const float valinsi = !logun ? initialval
				: logun->getSIValue( initialval );
	    const float convertedval = outun->getUserValueFromSI( valinsi );
	    log->valArr()[idl] = convertedval;
	}
    }

    return true;
}


void uiWellLogMerger::updateLogViewer()
{
    uiWellLogDisplay::LogData* wld = &logdisp_->logData();
    wld->setLog( outlogs_[0] );
    wld->disp_.color_ = Color::stdDrawColor( 1 );
    wld->zoverlayval_ = 1;
    logdisp_->reDraw();
}


void uiWellLogMerger::setOutputLogName( const char* nm )
{
    if ( nmfld_ )
	nmfld_->setText( nm );
}


const char* uiWellLogMerger::getOutputLogName() const
{
    return nmfld_ ? nmfld_->text() : 0;
}
