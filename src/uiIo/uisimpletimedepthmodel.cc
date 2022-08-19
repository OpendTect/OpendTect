/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisimpletimedepthmodel.h"

#include "binidvalue.h"
#include "od_iostream.h"
#include "simpletimedepthmodel.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uiioobjseldlg.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uitblimpexpdatasel.h"
#include "uizrangeinput.h"
#include "unitofmeasure.h"


#define mTimeCol 0
#define mDepthCol 1


class uiImpTimeDepthTable : public uiDialog
{ mODTextTranslationClass(uiImpTimeDepthTable);
public:
uiImpTimeDepthTable( uiParent* p, SimpleTimeDepthModel& mdl )
    : uiDialog(p,uiDialog::Setup(tr("Read Time-Depth data"),
				mNoDlgTitle,mNoHelpKey))
    , fd_(*SimpleTimeDepthAscIO::getDesc(true))
    , mdl_(mdl)
{
    filefld_ = new uiASCIIFileInput( this, true );
    fmtfld_ = new uiTableImpDataSel( this, fd_, mNoHelpKey );
    fmtfld_->attach( alignedBelow, filefld_ );
}

protected:

bool acceptOK( CallBacker* ) override
{
    const BufferString fnm( filefld_->fileName() );
    if ( fnm.isEmpty() || !fmtfld_->commit() )
	return false;

    od_istream strm( fnm );
    SimpleTimeDepthAscIO ascio( fd_ );
    return ascio.get( strm, mdl_ );
}

    uiASCIIFileInput*		filefld_;
    uiTableImpDataSel*		fmtfld_;
    Table::FormatDesc		fd_;
    SimpleTimeDepthModel&	mdl_;
};


#define mDefNrRows 10


class uiSimpleTimeDepthTable : public uiDialog
{ mODTextTranslationClass(uiSimpleTimeDepthTable);
public:
uiSimpleTimeDepthTable( uiParent* p, const MultiID& mid )
    : uiDialog(p,uiDialog::Setup(tr("Simple Time-Depth Model"),mNoDlgTitle,
				mTODOHelpKey))
    , mid_(mid)
{
    uiTable::Setup tblsu( mDefNrRows, 2 );
    tblsu.rowgrow( true ).insertrowallowed( true ).removerowallowed( true )
						  .selmode( uiTable::Multi );
    tbl_ = new uiTable( this,tblsu, "Time-Depth Table" );
    const uiString timelbl = toUiString("%1 %2").arg(uiStrings::sTWT()).
			arg(UnitOfMeasure::surveyDefTimeUnitAnnot(true,true));
    tbl_->setColumnLabel( mTimeCol, timelbl );
    tbl_->setColumnLabel( mDepthCol, ZDomain::Depth().getLabel() );

    addbut_ = new uiToolButton( this, "plus", tr("Add Row"),
				    mCB(this,uiSimpleTimeDepthTable,addCB) );
    addbut_->attach( rightOf, tbl_ );
    rembut_ = new uiToolButton( this, "minus", tr("Add Row"),
				    mCB(this,uiSimpleTimeDepthTable,removeCB) );
    rembut_->attach( alignedBelow, addbut_ );
    if ( mid.isUdf() )
    {
	IOObjContext ctxt = mIOObjContext(SimpleTimeDepthModel);
	ctxt.forread_ = false;
	savefld_ = new uiIOObjSel( this, ctxt, uiStrings::sSaveAs() );
    }
    else
    {
	SimpleTimeDepthModel mdl( mid_ );
	fillTable( mdl );
    }

    auto* butgrp = new uiGroup( this, "ButtonGroup" );
    if ( savefld_ )
	butgrp->attach( stretchedAbove, savefld_ );

    butgrp->attach( alignedBelow, tbl_ );
    auto* impbut = new uiPushButton( butgrp, tr("Read file"),
	    			mCB(this,uiSimpleTimeDepthTable,readCB), false);

    twtfld_ = new uiCheckBox( butgrp, tr("Time is TWT") );
    mAttachCB( twtfld_->activated, uiSimpleTimeDepthTable::timeTypeChngCB );
    twtfld_->attach( rightBorder );
    twtfld_->setChecked( true );
    zinfeet_ = new uiCheckBox( butgrp, tr("Z in feet") );
    mAttachCB( zinfeet_->activated, uiSimpleTimeDepthTable::zUnitChngCB );
    zinfeet_->attach( leftOf, twtfld_ );
    zinfeet_->attach( rightTo, impbut );
    zinfeet_->setChecked( SI().zInFeet() );

    mAttachCB( postFinalize(), uiSimpleTimeDepthTable::timeTypeChngCB );
}


~uiSimpleTimeDepthTable()
{
    detachAllNotifiers();
}

MultiID getKey() const
{ return savefld_->key(); }

protected:

void fillTable( const SimpleTimeDepthModel& mdl )
{
    tbl_->clearTable();
    const TypeSet<float>& times = mdl.getRawTimes();
    const TypeSet<float>& depths = mdl.getRawDepths();
    const int nrpts = times.size();
    tbl_->setNrRows( nrpts );
    for ( int idx=0; idx<nrpts; idx++ )
    {
	tbl_->setValue( RowCol(idx,mTimeCol), times[idx] );
	tbl_->setValue( RowCol(idx,mDepthCol), depths[idx] );
    }
}


void removeCB( CallBacker* cb )
{
    if ( tbl_->nrRows() < 2 )
	return;

    tbl_->removeRow( tbl_->nrRows() - 1 );
    rembut_->setSensitive( tbl_->nrRows() > 1 );
}

void addCB( CallBacker* cb )
{
    tbl_->setNrRows( tbl_->nrRows() + 1 );
    rembut_->setSensitive( tbl_->nrRows() > 1 );
}


int getNrDataPoints() const
{
    return tbl_->nrRows();
}


void readCB( CallBacker* )
{
    SimpleTimeDepthModel mdl;
    uiImpTimeDepthTable dlg( this, mdl );
    if ( dlg.go() )
	fillTable( mdl );
}


static const UnitOfMeasure* getDisplayUnit( bool zinfeet )
{
    return zinfeet ? UoMR().get( "Feet" ) : UoMR().get( "Meter" );
}


void zUnitChngCB( CallBacker* )
{
    const bool zinfeet = zinfeet_->isChecked();
    const uiString depthunit =
			    uiStrings::sDistUnitString( zinfeet, true, true );
    const uiString dispstr = tr("Depth %1").arg( depthunit );
    tbl_->setColumnLabel( mDepthCol, dispstr );
    if ( tbl_->isEmpty() )
	return;

    const UnitOfMeasure* newuomr = getDisplayUnit( zinfeet );
    const UnitOfMeasure* olduomr = getDisplayUnit( !zinfeet );

    for ( int idx=0; idx<getNrDataPoints(); idx++ )
    {
	const RowCol rc( idx, mDepthCol );
	const float oldval = tbl_->getFValue( rc );
	if ( mIsUdf(oldval) )
	    continue;

	const float val = getConvertedValue( oldval, olduomr, newuomr );
	tbl_->setValue( rc, val );
    }
}


void timeTypeChngCB( CallBacker* )
{
    const bool istymtwt = twtfld_->isChecked();
    uiString timestr = istymtwt ? uiStrings::sTWT() : uiStrings::sOWT();
    const uiString& timelbl = timestr.append(
			    UnitOfMeasure::surveyDefTimeUnitAnnot(true,true) );
    tbl_->setColumnLabel( mTimeCol, timelbl );
    if ( tbl_->isEmpty() )
	return;

    const float convfac = istymtwt ? 2 : 0.5;
    for ( int idx=0; idx<getNrDataPoints(); idx++ )
    {
	const RowCol rc( idx, mTimeCol );
	const float oldval = tbl_->getFValue( rc );
	if ( mIsUdf(oldval) )
	    continue;

	tbl_->setValue( rc, oldval*convfac );
    }

}

bool acceptOK( CallBacker* ) override
{
    TypeSet<float> times;
    TypeSet<float> depths;
    const float timefac = ZDomain::Time().userFactor();
    const float depthfac = SI().depthsInFeet() ? mFromFeetFactorF : 1.f;
    for ( int idx=0; idx<tbl_->nrRows(); idx++ )
    {
	const float timeval = tbl_->getFValue( RowCol(idx,mTimeCol) );
	const float depthval = tbl_->getFValue( RowCol(idx,mDepthCol) );
	if ( mIsUdf(timeval) || mIsUdf(depthval) )
	    continue;

	times += timeval / timefac;
	depths += depthfac * depthval;
    }

    SimpleTimeDepthModel mdl;
    mdl.setRawData( times, depths );

    if ( mid_.isUdf() )
    {
	const IOObj* outobj = savefld_->ioobj();
	if ( !outobj )
	    return false;
	mid_ = outobj->key();
    }

    mdl.save( mid_ );
    return true;
}

    uiTable*		tbl_;
    uiIOObjSel*		savefld_    = nullptr;
    uiCheckBox*		zinfeet_    = nullptr;
    uiCheckBox*		twtfld_     = nullptr;
    uiToolButton*	addbut_     = nullptr;
    uiToolButton*	rembut_     = nullptr;

    MultiID		mid_;
};

uiSimpleTimeDepthTransform::uiSimpleTimeDepthTransform( uiParent* p, bool t2d )
    : uiTime2DepthZTransformBase(p,t2d)
    , transform_(nullptr)
{
    selfld_ = new uiIOObjSel( this, mIOObjContext(SimpleTimeDepthModel),
			      uiString::emptyString() );
    mAttachCB( selfld_->selectionDone,
	       uiSimpleTimeDepthTransform::setZRangeCB );

    auto* editcreatebut = new uiPushButton( this,
			tr("Create/Edit"), false );
    editcreatebut->attach( rightOf, selfld_ );
    auto* mnu = new uiMenu;
    mnu->insertAction( new uiAction(uiStrings::sCreate(),
		mCB(this,uiSimpleTimeDepthTransform,createCB),"create") );
    mnu->insertAction( new uiAction(uiStrings::sEdit(),
		mCB(this,uiSimpleTimeDepthTransform,editCB),"edit") );
    editcreatebut->setMenu( mnu );

    setHAlignObj( selfld_ );
}


uiSimpleTimeDepthTransform::~uiSimpleTimeDepthTransform()
{
    detachAllNotifiers();
    unRefAndZeroPtr( transform_ );
}


ZAxisTransform* uiSimpleTimeDepthTransform::getSelection()
{
    unRefAndZeroPtr( transform_ );

    const IOObj* ioobj = selfld_->ioobj( true );
    if ( !ioobj )
	return nullptr;

    if ( t2d_ )
	transform_ = new SimpleT2DTransform( ioobj->key() );
    else
	transform_ = new SimpleD2TTransform( ioobj->key() );

    refPtr( transform_ );
    if ( !transform_ || !transform_->isOK() )
    {
	unRefAndZeroPtr( transform_ );
	return nullptr;
    }

    return transform_;
}


void uiSimpleTimeDepthTransform::createCB( CallBacker* )
{
    uiSimpleTimeDepthTable dlg( this, MultiID::udf() );
    if ( !dlg.go() )
	return;
    
    selfld_->setInput( dlg.getKey() );
    setZRangeCB( nullptr );
}


void uiSimpleTimeDepthTransform::setZRangeCB( CallBacker* )
{
    RefMan<ZAxisTransform> trans = getSelection();
    if ( !rangefld_ )
	return;

    if ( !rangechanged_ )
    {
	StepInterval<float> range( StepInterval<float>::udf() );
	if ( trans )
	{
	    range = trans->getZInterval( false );
	    range.step = trans->getGoodZStep();
	    if ( range.isUdf() )
		range.setUdf();
	}

	rangefld_->setZRange( range );
    }
}


void uiSimpleTimeDepthTransform::editCB( CallBacker* )
{
    const IOObj* ioobj = selfld_->ioobj( true );
    if ( !ioobj )
    {
	uiMSG().error( tr("Cannot read object from store") );
	return;
    }


    const MultiID mid = ioobj->key();
    if ( mid.isUdf() )
	return;

    uiSimpleTimeDepthTable dlg( this, mid );
    if ( !dlg.go() )
	return;

    selfld_->setInput( mid );
    setZRangeCB( nullptr );
}


bool uiSimpleTimeDepthTransform::acceptOK()
{
    if ( !selfld_->ioobj(false) )
	return false;

    if ( !transform_ )
    {
	uiMSG().error(
		uiStrings::phrCannotCreate(tr("simple time-depth transform")) );
	return false;
    }

    return true;
}



void uiSimpleTimeDepthTransform::initClass()
{
    uiZAxisTransform::factory().addCreator( createInstance,
				    SimpleT2DTransform::sFactoryKeyword(),
				    tr("Simple Time-Depth model"));
}


uiZAxisTransform* uiSimpleTimeDepthTransform::createInstance(uiParent* p,
				const char* fromdomain, const char* todomain )
{
    if ( !fromdomain || !todomain )
	return nullptr;

    if ( fromdomain==ZDomain::sKeyTime() && todomain==ZDomain::sKeyDepth() )
	return new uiSimpleTimeDepthTransform( p, true );
    else if ( fromdomain==ZDomain::sKeyDepth() && todomain==ZDomain::sKeyTime())
	return new uiSimpleTimeDepthTransform( p, false );

    return nullptr;
}
