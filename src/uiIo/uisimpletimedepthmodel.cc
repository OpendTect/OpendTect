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


class uiSimpleTimeDepthTable : public uiDialog
{ mODTextTranslationClass(uiSimpleTimeDepthTable);
public:
uiSimpleTimeDepthTable( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Simple Time-Depth Model"),mNoDlgTitle,
				mTODOHelpKey))
{
    uiTable::Setup tblsu( 10, 2 );
    tbl_ = new uiTable( this,tblsu, "Time-Depth Table" );
    tbl_->setColumnLabel( mTimeCol, ZDomain::Time().getLabel() );
    tbl_->setColumnLabel( mDepthCol, ZDomain::Depth().getLabel() );

    uiButton* openbut = new uiToolButton( this, "open", uiStrings::sOpen(),
	    			mCB(this,uiSimpleTimeDepthTable,openCB) );
    uiButton* readbut = new uiPushButton( this, tr("Read file"),
	    			mCB(this,uiSimpleTimeDepthTable,readCB), false);
    openbut->attach( alignedBelow, tbl_ );
    readbut->attach( rightOf, openbut );

    IOObjContext ctxt = mIOObjContext(SimpleTimeDepthModel);
    ctxt.forread_ = false;
    savefld_ = new uiIOObjSel( this, ctxt, uiStrings::sSaveAs() );
    savefld_->attach( alignedBelow, openbut );
}

MultiID getKey() const
{ return savefld_->key(); }

protected:

void fillTable( const SimpleTimeDepthModel& mdl )
{
    tbl_->clearTable();
    const float timefac = ZDomain::Time().userFactor();
    const float depthfac = SI().depthsInFeet() ? mToFeetFactorF : 1.f;
    const TypeSet<float>& times = mdl.getRawTimes();
    const TypeSet<float>& depths = mdl.getRawDepths();
    for ( int idx=0; idx<times.size(); idx++ )
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

    const IOObj* outobj = savefld_->ioobj();
    if ( !outobj )
	return false;

    mdl.save( outobj->key() );
    return true;
}

    uiTable*		tbl_;
    uiIOObjSel*		savefld_;

};

uiSimpleTimeDepthTransform::uiSimpleTimeDepthTransform( uiParent* p )
    : uiTime2DepthZTransformBase(p, true )
    , transform_( 0 )
{
    selfld_ = new uiIOObjSel( this, mIOObjContext(SimpleTimeDepthModel), 
			   uiString::emptyString() );
    selfld_->selectionDone.notify( mCB(this,uiSimpleTimeDepthTransform,
					setZRangeCB) );

    uiPushButton* createbut = new uiPushButton( this, uiStrings::sCreate(),
	    					false );
    createbut->attach( rightOf, selfld_ );
    createbut->activated.notify( mCB(this,uiSimpleTimeDepthTransform,createCB));

    setHAlignObj( selfld_ );
}


uiSimpleTimeDepthTransform::~uiSimpleTimeDepthTransform()
{
    unRefAndZeroPtr( transform_ );
}


ZAxisTransform* uiSimpleTimeDepthTransform::getSelection()
{
    unRefAndZeroPtr( transform_ );

    const IOObj* ioobj = selfld_->ioobj( true );
    if ( !ioobj ) return 0;

    transform_ = new SimpleTimeDepthTransform( ioobj->key() );
    refPtr( transform_ );
    if ( !transform_ || !transform_->isOK() )
    {
	unRefAndZeroPtr( transform_ );
	return 0;
    }

    return transform_;
}


void uiSimpleTimeDepthTransform::createCB( CallBacker* )
{
    uiSimpleTimeDepthTable dlg( this );
    if ( !dlg.go() )
	return;
    
    selfld_->setInput( dlg.getKey() );
    setZRangeCB( 0 );
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
				    SimpleTimeDepthTransform::sFactoryKeyword(),
				    tr("Simple Time-Depth model"));
}


uiZAxisTransform* uiSimpleTimeDepthTransform::createInstance(uiParent* p,
				const char* fromdomain, const char* todomain )
{
    return new uiSimpleTimeDepthTransform( p );
}
