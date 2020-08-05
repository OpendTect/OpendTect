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
#include "uitaskrunner.h"
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
	: uiDialog( p , uiDialog::Setup(tr("Merge logs"),
				getDlgUiTitle(wllids), mNoHelpKey)
				.oktext(uiStrings::sSave())
				.applybutton(true).applytext(tr("Merge")) )
    , wellids_( wllids )
    , havenew_(false)
{
    if ( !chosenlognms )
	needselectlogfld_ = true;
    else
	chosenlognms_.add( *chosenlognms, false );

    if ( wellids_.isEmpty() )
    {
	new uiLabel( this, tr("No wells.\nPlease import"
			      " or create a well first.") );
	setCtrlStyle( CloseOnly );
	return;
    }

    createMergeParaGrp();
    createMergedLogDispGrp();
}


void uiWellLogMerger::createMergeParaGrp()
{
    mergeparamgrp_ = new uiGroup( this, "merge parameters" );
    logfld_ = new uiGenInput( mergeparamgrp_,
			     uiStrings::phrOutput( tr("Select Log Type")),
			     StringListInpSpec(PropertyRef::StdTypeDef()) );
    mAttachCB( logfld_->valuechanged, uiWellLogMerger::logSetCB );

    extrapolatefld_ = new uiGenInput( mergeparamgrp_, tr("Extrapolate"),
					    BoolInpSpec(true) );
    extrapolatefld_->attach( alignedBelow, logfld_ );

    intrapolatefld_ = new uiGenInput( mergeparamgrp_, tr("Intrapolate"),
					    BoolInpSpec(true) );
    intrapolatefld_->attach( rightOf, extrapolatefld_ );

    overlapfld_ = new uiGenInput(mergeparamgrp_, tr("Overlap Action"),
					StringListInpSpec(
					Well::LogMerger::OverlapActionDef()) );
    overlapfld_->attach( alignedBelow, extrapolatefld_ ) ;
    mAttachCB( overlapfld_->valuechanged, uiWellLogMerger::overlapCB );

    if ( needselectlogfld_ )
    {
	uiListBox::Setup sul( OD::ChooseAtLeastOne, tr("Select logs"),
						    uiListBox::AboveMid );
	loglistfld_ = new uiListBox( mergeparamgrp_, sul );
	loglistfld_->attach( alignedBelow, overlapfld_ );

	selbuttons_ = new uiGroup( mergeparamgrp_, "select buttons" );
	uiLabel* sellbl = new uiLabel( selbuttons_, uiStrings::sSelect() );
	CallBack cbs = mCB(this,uiWellLogMerger,selButPush);
	toselect_ = new uiToolButton( selbuttons_, uiToolButton::RightArrow,
					    tr("Move right"), cbs );
	toselect_->attach( centeredBelow, sellbl );
	toselect_->setHSzPol( uiObject::Undef );
	fromselect_ = new uiToolButton( selbuttons_, uiToolButton::LeftArrow,
					    tr("Move left"), cbs );
	fromselect_->attach( alignedBelow, toselect_ );
	fromselect_->setHSzPol( uiObject::Undef );
	selbuttons_->setHAlignObj( toselect_ );
	selbuttons_->attach( centeredRightOf, loglistfld_ );
    }

    uiListBox::Setup susl( OD::ChooseNone, tr("Selected logs"),
						uiListBox::AboveMid );
    selectedlogsfld_ = new uiListBox( mergeparamgrp_, susl );
    if ( needselectlogfld_ )
	selectedlogsfld_->attach( centeredRightOf, selbuttons_ );
    else
    {
	selectedlogsfld_->addItems( chosenlognms_ );
	selectedlogsfld_->attach( alignedBelow, overlapfld_ );
    }

    movebuttons_ = new uiGroup( mergeparamgrp_, "move buttons" );
    uiLabel* movelbl = new uiLabel( movebuttons_, tr("Change \n priority") );
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

    uiSeparator* sep = new uiSeparator( mergeparamgrp_, "sep" );
    if ( needselectlogfld_ )
	sep->attach( ensureBelow, loglistfld_ );
    sep->attach( ensureBelow, selectedlogsfld_ );

    float defsr = SI().depthsInFeet() ? 0.5f : 0.15f;
    srfld_ = new uiGenInput( mergeparamgrp_,
			     uiStrings::phrOutput( tr("sample distance")),
			     FloatInpSpec(defsr) );
    srfld_->attach( ensureBelow, sep );
    srfld_->attach( alignedBelow, overlapfld_ );

    nmfld_ = new uiGenInput( mergeparamgrp_, tr("Name for new log") );
    nmfld_->attach( alignedBelow, srfld_ );

    uiUnitSel::Setup uussu( PropertyRef::Other, uiStrings::phrOutput(
						       tr("unit of measure")) );
    uussu.withnone( true );
    outunfld_ = new uiUnitSel( mergeparamgrp_, uussu );
    outunfld_->attach( alignedBelow, nmfld_ );

    movebuttons_->display( false );

    mAttachCB( applyPushed, uiWellLogMerger::applyCB );
}


void uiWellLogMerger::createMergedLogDispGrp()
{
    mergedlogdispgrp_ = new uiGroup( this, "merged log display" );
    mergedlogdispgrp_->attach( rightOf, mergeparamgrp_ );

    uiWellLogDisplay::Setup wldsu;
    logdisp_ = new uiWellLogDisplay( mergedlogdispgrp_, wldsu );
    logdisp_->setPrefWidth( 150 );
    logdisp_->setPrefHeight( 450 );

    changelogdispbut_ = new uiGroup( mergedlogdispgrp_, "change log" );
    CallBack cbm = mCB(this,uiWellLogMerger,changeLogDispButPush);
    prevlog_ = new uiToolButton( changelogdispbut_, uiToolButton::LeftArrow,
				    tr("Previous log"), cbm );
    prevlog_->setHSzPol( uiObject::Undef );
    nextlog_ = new uiToolButton( changelogdispbut_, uiToolButton::RightArrow,
					tr("Next log"), cbm );
    nextlog_->attach( rightOf, prevlog_ );
    nextlog_->setHSzPol( uiObject::Undef );
    changelogdispbut_->setHAlignObj( prevlog_ );
    changelogdispbut_->attach( centeredBelow, logdisp_ );
}


uiWellLogMerger::~uiWellLogMerger()
{
    detachAllNotifiers();
    delete logdisp_;
}


void uiWellLogMerger::logSetCB( CallBacker* cb )
{
    const int logsel = logfld_->getIntValue();
    PropertyRef::StdType logprop = PropertyRef::StdTypeDef().
						getEnumForIndex( logsel );
    outunfld_->setPropType(logprop);
    if ( needselectlogfld_ )
    {
	BufferStringSet lognms;
	for ( int idx=0; idx<wellids_.size(); idx++ )
	{
	    Well::Data* wd = Well::MGR().get( wellids_[idx] );
	    TypeSet<int> lgids;
	    lgids = wd->logs().getSuitable(logprop);
	    for ( int idl=0; idl<lgids.size(); idl++ )
		lognms.addIfNew( wd->logs().getLog(lgids[idl]).name() );
	}
	loglistfld_->setEmpty();
	loglistfld_->addItems( lognms );
    }
    else
    {
	selectedlogsfld_->setEmpty();
	selectedlogsfld_->addItems( chosenlognms_ );
    }
}


void uiWellLogMerger::overlapCB( CallBacker* cb )
{
    const int overlapaction = overlapfld_->getIntValue();
    action_= Well::LogMerger::OverlapActionDef().getEnumForIndex(overlapaction);
    if ( action_ == Well::LogMerger::UseAverage )
	movebuttons_->display( false );
    else if ( action_ == Well::LogMerger::UseOneLog )
	movebuttons_->display( true );
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
    int index = outlgs_.indexOf( wld->log() );
    if ( but == nextlog_ && index<outlgs_.size()-1 )
    {
	wld->setLog( outlgs_[index+1] );
	logdisp_->reDraw();
    }

    if ( but == prevlog_ && index>0 )
    {
	wld->setLog( outlgs_[index-1] );
	logdisp_->reDraw();
    }
}


void uiWellLogMerger::applyCB( CallBacker* )
{
    if ( action_ == Well::LogMerger::UseAverage  && needselectlogfld_ )
	loglistfld_->getChosen( chosenlognms_ );

    const BufferString newnm = nmfld_ ? nmfld_->text() : "";
    if ( newnm.isEmpty() )
	uiMSG().message( tr("Please provide a name for the new log") );
    if ( chosenlognms_.isPresent(newnm) )
	uiMSG().message( tr("A log with this name already exists."
		"\nPlease enter a different name for the new log") );

    zsampling_ = srfld_->getfValue();
    if ( mIsUdf(zsampling_) )
	uiMSG().message(
		tr("Please provide the Z sample rate for the  output log") );

    outlgs_.setEmpty();
    merge();
    convertMergedLog();
    updateLogViewer();
}


bool uiWellLogMerger::acceptOK( CallBacker* )
{
    int errcount = 0;
    for ( int idx=0; idx<outlgs_.size(); idx++ )
    {
	if ( !writeNewLog() )
	{
	    uiMSG().message( tr("Failed to add %1 to %2")
				.arg(outlgs_[idx]->name())
				.arg(Well::MGR().get(wellids_[idx])->name()) );
	    errcount += 1;
	}
    }

    if ( errcount != outlgs_.size() )
    {
	uiMSG().message( tr("Successfully added new log") );
	havenew_ = true;
    }
    else
	return false;

    return true;
}


void uiWellLogMerger::merge()
{
    TaskGroup lmtg;
    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	Well::Log* outlg = new Well::Log( getOutputLogName() );
	outlgs_.add( outlg );
	auto* lgmerger = new Well::LogMerger( wellids_[idx],
							chosenlognms_,
							*outlgs_[idx] );
	lgmerger->setSamplingDist( zsampling_ );
	lgmerger->setOverlapAction( action_ );
	lgmerger->setDoInterpolation( intrapolatefld_->getBoolValue() );
	lgmerger->setDoExtrapolation( extrapolatefld_->getBoolValue() );
	lmtg.addTask(lgmerger);
    }

    uiTaskRunner uitr( this );
    if ( !uitr.execute(lmtg) )
	return;
}


bool uiWellLogMerger::writeNewLog()
{
    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	Well::MGR().get(wellids_[idx])->logs().add( outlgs_[idx] );
	Well::Writer wtr( wellids_[idx], *Well::MGR().get( wellids_[idx] ) );
	if ( !wtr.putLog(*outlgs_[idx]) )
	    return false;
    }

    return true;
}


void uiWellLogMerger::convertMergedLog()
{
    const UnitOfMeasure* outun = outunfld_->getUnit();
    if ( outun )
    {
	for ( int idx=0; idx<outlgs_.size(); idx++ )
	{
	    const UnitOfMeasure* logun = outlgs_[idx]->unitOfMeasure();
	    for ( int idl=0; idl<outlgs_[idx]->size(); idl++  )
	    {
		const float initialval = outlgs_[idx]->value( idl );
		const float valinsi = !logun ? initialval
				    : logun->getSIValue( initialval );
		const float convertedval = outun->getUserValueFromSI( valinsi );
		outlgs_[idx]->valArr()[idl] = convertedval;
	    }
	}
    }
}


void uiWellLogMerger::updateLogViewer()
{
    uiWellLogDisplay::LogData* wld = &logdisp_->logData();
    wld->setLog( outlgs_[0] );
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
