/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisimpletimedepthmodel.h"

#include "od_istream.h"
#include "survinfo.h"
#include "zdomain.h"

#include "uibutton.h"
#include "uidialog.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
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

private:

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
				mODHelpKey(mSimpleTimeDepthTableHelpID)))
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
    tbl_->setColumnToolTip( mDepthCol, tr("TVDSS Depths") );

    addbut_ = new uiToolButton( this, "plus", tr("Add Row"),
				    mCB(this,uiSimpleTimeDepthTable,addCB) );
    addbut_->attach( rightOf, tbl_ );
    rembut_ = new uiToolButton( this, "minus", tr("Add Row"),
				    mCB(this,uiSimpleTimeDepthTable,removeCB) );
    rembut_->attach( alignedBelow, addbut_ );

    auto* butgrp = new uiGroup( this, "ButtonGroup" );
    auto* readbut = new uiPushButton( butgrp, tr("Read file"),
		      mCB(this,uiSimpleTimeDepthTable,readCB), false );

    twtfld_ = new uiCheckBox( butgrp, tr("Time is TWT") );
    mAttachCB( twtfld_->activated, uiSimpleTimeDepthTable::timeTypeChngCB );
    twtfld_->attach( rightTo, readbut );
    twtfld_->setChecked( true );

    zinfeet_ = new uiCheckBox( butgrp, tr("Z in feet") );
    mAttachCB( zinfeet_->activated, uiSimpleTimeDepthTable::zUnitChngCB );
    zinfeet_->attach( rightBorder );
    zinfeet_->setChecked( SI().depthsInFeet() );

    twtfld_->attach( leftOf, zinfeet_ );

    if ( mid.isUdf() )
    {
	IOObjContext ctxt = mIOObjContext(SimpleTimeDepthModel);
	ctxt.forread_ = false;
	savefld_ = new uiIOObjSel( this, ctxt, uiStrings::sSaveAs() );
	butgrp->attach( stretchedAbove, savefld_ );
    }

    butgrp->attach( alignedBelow, tbl_ );

    mAttachCB( postFinalize(), uiSimpleTimeDepthTable::initDlg );
}


~uiSimpleTimeDepthTable()
{
    detachAllNotifiers();
}


MultiID getKey() const
{
    return savefld_->key();
}

private:

int getNrDataPoints() const
{
    return tbl_->nrRows();
}


static const UnitOfMeasure* getDepthDisplayUnit( bool zinfeet )
{
    return zinfeet ? UoMR().get( "Feet" ) : UoMR().get( "Meter" );
}


void fillTable( const SimpleTimeDepthModel& mdl )
{
    tbl_->clearTable();
    const TypeSet<float>& times = mdl.getRawTimes();
    const TypeSet<float>& depths = mdl.getRawDepths();
    const int nrpts = times.size();
    tbl_->setNrRows( nrpts );

    const float twtfac = twtfld_->isChecked() ? 1.f : 0.5f;
    const UnitOfMeasure* tuom = SimpleTimeDepthModel::getTimeUnit();
    const UnitOfMeasure* zuom = SimpleTimeDepthModel::getDepthUnit();
    const UnitOfMeasure* tbltuom = UnitOfMeasure::surveyDefTimeUnit();
    const UnitOfMeasure* tblzuom = getDepthDisplayUnit( zinfeet_->isChecked() );
    for ( int idx=0; idx<nrpts; idx++ )
    {
	const float twtval = twtfac *
			     getConvertedValue( times[idx], tuom, tbltuom );
	const float zval = getConvertedValue( depths[idx], zuom, tblzuom );
	tbl_->setValue( RowCol(idx,mTimeCol), twtval );
	tbl_->setValue( RowCol(idx,mDepthCol), zval );
    }
}


void initDlg( CallBacker* )
{
    if ( mid_.isUdf() )
    {
	const UnitOfMeasure* tblzuom = getDepthDisplayUnit(
						zinfeet_->isChecked() );
	const float firstz = getConvertedValue( -SI().seismicReferenceDatum(),
				    UnitOfMeasure::surveyDefSRDStorageUnit(),
				    tblzuom );
	tbl_->setValue( RowCol(0,mTimeCol), 0.f );
	tbl_->setValue( RowCol(0,mDepthCol), firstz );
    }
    else
    {
	const SimpleTimeDepthModel mdl( mid_ );
	fillTable( mdl );
    }
}


void addCB( CallBacker* )
{
    tbl_->setNrRows( tbl_->nrRows() + 1 );
    rembut_->setSensitive( tbl_->nrRows() > 1 );
}


void removeCB( CallBacker* )
{
    if ( tbl_->nrRows() < 2 )
	return;

    tbl_->removeRow( tbl_->nrRows() - 1 );
    rembut_->setSensitive( tbl_->nrRows() > 1 );
}


void readCB( CallBacker* )
{
    SimpleTimeDepthModel mdl;
    uiImpTimeDepthTable dlg( this, mdl );
    if ( dlg.go() == uiDialog::Accepted )
	fillTable( mdl );
}


void timeTypeChngCB( CallBacker* cb )
{
    if ( cb != twtfld_ )
    {
	pErrMsg("Callbacker should not be used in this way");
	return;
    }

    const bool istymtwt = twtfld_->isChecked();
    uiString timestr = istymtwt ? uiStrings::sTWT() : uiStrings::sOWT();
    const uiString& timelbl = timestr.append(
			    UnitOfMeasure::surveyDefTimeUnitAnnot(true,true) );
    tbl_->setColumnLabel( mTimeCol, timelbl );
    if ( tbl_->isEmpty() )
	return;

    const float convfac = istymtwt ? 2.f : 0.5f;
    for ( int idx=0; idx<getNrDataPoints(); idx++ )
    {
	const RowCol rc( idx, mTimeCol );
	const float oldval = tbl_->getFValue( rc );
	if ( mIsUdf(oldval) )
	    continue;

	tbl_->setValue( rc, oldval*convfac );
    }
}


void zUnitChngCB( CallBacker* cb )
{
    if ( cb != zinfeet_ )
    {
	pErrMsg("Callbacker should not be used in this way");
	return;
    }

    const bool zinfeet = zinfeet_->isChecked();
    const uiString depthunit =
			    uiStrings::sDistUnitString( zinfeet, true, true );
    const uiString dispstr = tr("Depth %1").arg( depthunit );
    tbl_->setColumnLabel( mDepthCol, dispstr );
    if ( tbl_->isEmpty() )
	return;

    const UnitOfMeasure* olduomr = getDepthDisplayUnit( !zinfeet );
    const UnitOfMeasure* newuomr = getDepthDisplayUnit( zinfeet );
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


bool acceptOK( CallBacker* ) override
{
    TypeSet<float> times;
    TypeSet<float> depths;
    const float twtfac = twtfld_->isChecked() ? 1.f : 2.f;
    const UnitOfMeasure* tbltuom = UnitOfMeasure::surveyDefTimeUnit();
    const UnitOfMeasure* tblzuom = getDepthDisplayUnit( zinfeet_->isChecked() );
    const UnitOfMeasure* tuom = SimpleTimeDepthModel::getTimeUnit();
    const UnitOfMeasure* zuom = SimpleTimeDepthModel::getDepthUnit();
    for ( int idx=0; idx<tbl_->nrRows(); idx++ )
    {
	const float timeval = tbl_->getFValue( RowCol(idx,mTimeCol) );
	const float depthval = tbl_->getFValue( RowCol(idx,mDepthCol) );
	if ( mIsUdf(timeval) || mIsUdf(depthval) )
	    continue;

	times += twtfac * getConvertedValue( timeval, tbltuom, tuom );
	depths += getConvertedValue( depthval, tblzuom, zuom );
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

    return mdl.save( mid_ );
}

    uiTable*		tbl_;
    uiToolButton*	addbut_     = nullptr;
    uiToolButton*	rembut_     = nullptr;
    uiCheckBox*		zinfeet_    = nullptr;
    uiCheckBox*		twtfld_     = nullptr;
    uiIOObjSel*		savefld_    = nullptr;

    MultiID		mid_;
};


// uiSimpleTimeDepthTransform

uiSimpleTimeDepthTransform::uiSimpleTimeDepthTransform( uiParent* p, bool t2d )
    : uiTime2DepthZTransformBase(p,t2d)
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
}


void uiSimpleTimeDepthTransform::doInitGrp()
{
    setZRangeCB( nullptr );
}


ZAxisTransform* uiSimpleTimeDepthTransform::getSelection()
{
    transform_ = nullptr;
    const IOObj* ioobj = selfld_->ioobj( true );
    if ( !ioobj )
	return nullptr;

    if ( isTimeToDepth() )
	transform_ = new SimpleT2DTransform( ioobj->key() );
    else
	transform_ = new SimpleD2TTransform( ioobj->key() );

    if ( !transform_ || !transform_->isOK() )
	transform_ = nullptr;

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
    ConstRefMan<ZAxisTransform> trans = getSelection();
    if ( !rangefld_ )
	return;

    if ( !rangechanged_ )
    {
	ZSampling range = ZSampling::udf();
	if ( trans )
	    range = trans->getZInterval( false );

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


uiZAxisTransform* uiSimpleTimeDepthTransform::createInstance( uiParent* p,
				const uiZAxisTranformSetup& setup )
{
    if ( setup.fromdomain_.isEmpty() || setup.todomain_.isEmpty() )
	return nullptr;

    if ( setup.fromdomain_ == ZDomain::sKeyTime() &&
				setup.todomain_ == ZDomain::sKeyDepth() )
	return new uiSimpleTimeDepthTransform( p, true );
    else if ( setup.fromdomain_ == ZDomain::sKeyDepth() &&
				    setup.todomain_ == ZDomain::sKeyTime() )
	return new uiSimpleTimeDepthTransform( p, false );

    return nullptr;
}
