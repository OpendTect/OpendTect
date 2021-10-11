/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Jan 2021
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
#include "uimsg.h"
#include "uitable.h"
#include "uitoolbutton.h"
#include "uitblimpexpdatasel.h"
#include "uizrangeinput.h"


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

bool acceptOK( CallBacker* )
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
    tbl_->setColumnLabel( mTimeCol, ZDomain::Time().getLabel() );
    tbl_->setColumnLabel( mDepthCol, ZDomain::Depth().getLabel() );

    addbut_ = new uiToolButton( this, "plus", tr("Add Row"),
				    mCB(this,uiSimpleTimeDepthTable,addCB) );
    addbut_->attach( rightOf, tbl_ );
    rembut_ = new uiToolButton( this, "minus", tr("Add Row"),
				    mCB(this,uiSimpleTimeDepthTable,removeCB) );
    rembut_->attach( alignedBelow, addbut_ );

    if ( mid.isUdf() )
    {
	auto* openbut = new uiToolButton( this, "open", uiStrings::sOpen(),
				mCB(this,uiSimpleTimeDepthTable,openCB) );
	auto* readbut = new uiPushButton( this, tr("Read file"),
			    mCB(this,uiSimpleTimeDepthTable,readCB), false);
	openbut->attach( alignedBelow, tbl_ );
	readbut->attach( rightOf, openbut );
	IOObjContext ctxt = mIOObjContext(SimpleTimeDepthModel);
	ctxt.forread_ = false;
	savefld_ = new uiIOObjSel( this, ctxt, uiStrings::sSaveAs() );
	savefld_->attach( alignedBelow, openbut );
    }
    else
    {
	SimpleTimeDepthModel mdl( mid_ );
	fillTable( mdl );
    }
}

MultiID getKey() const
{
    return savefld_ ? savefld_->key() : mid_;
}

protected:

void fillTable( const SimpleTimeDepthModel& mdl )
{
    tbl_->clearTable();
    const float timefac = ZDomain::Time().userFactor();
    const float depthfac = SI().depthsInFeet() ? mToFeetFactorF : 1.f;
    const TypeSet<float>& times = mdl.getRawTimes();
    const TypeSet<float>& depths = mdl.getRawDepths();
    const int nrpts = times.size();
    tbl_->setNrRows( nrpts );

    for ( int idx=0; idx<nrpts; idx++ )
    {
	tbl_->setValue( RowCol(idx,mTimeCol), timefac*times[idx] );
	tbl_->setValue( RowCol(idx,mDepthCol), depthfac*depths[idx] );
    }
}

void openCB( CallBacker* )
{
    CtxtIOObj ctxt( mIOObjContext(SimpleTimeDepthModel) );
    uiIOObjSelDlg dlg( this, ctxt );
    if ( !dlg.go() )
	return;

    SimpleTimeDepthModel mdl( dlg.chosenID() );
    fillTable( mdl ); 
}

void readCB( CallBacker* )
{
    SimpleTimeDepthModel mdl;
    uiImpTimeDepthTable dlg( this, mdl );
    if ( dlg.go() )
	fillTable( mdl );
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


bool acceptOK( CallBacker* )
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
    uiIOObjSel*		savefld_ ;
    MultiID		mid_;
    uiToolButton*	addbut_;
    uiToolButton*	rembut_;
};


uiSimpleTimeDepthTransform::uiSimpleTimeDepthTransform( uiParent* p, bool t2d )
    : uiTime2DepthZTransformBase(p,t2d)
    , transform_(nullptr)
{
    selfld_ = new uiIOObjSel( this, mIOObjContext(SimpleTimeDepthModel), 
			   uiString::emptyString() );
    mAttachCB( selfld_->selectionDone,
				    uiSimpleTimeDepthTransform::setZRangeCB );

    auto* createbut = new uiPushButton( this, uiStrings::sCreate(), false );
    createbut->attach( rightOf, selfld_ );
    mAttachCB( createbut->activated, uiSimpleTimeDepthTransform::createCB );

    auto* editbut = new uiPushButton( this, uiStrings::sEdit(), false );
    editbut->attach( rightOf, createbut );
    mAttachCB( editbut->activated, uiSimpleTimeDepthTransform::editCB );
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


void uiSimpleTimeDepthTransform::editCB( CallBacker* )
{
    const IOObj* ioobj = selfld_->ioobj( true );
    if ( !ioobj )
    {
	uiMSG().error( tr("Cannot read object from store") );
	return;
    }


    const MultiID mid = ioobj->key();
    if ( !mid.isUdf() && !mid.isEmpty() )
    {
	uiSimpleTimeDepthTable dlg( this, mid );
	if ( !dlg.go() )
	    return;

	selfld_->setInput( mid );
	setZRangeCB( nullptr );
    }
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
	    if ( range.isUdf() ) range.setUdf();
	}

	rangefld_->setZRange( range );
    }
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
