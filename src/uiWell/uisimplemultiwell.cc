/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uisimplemultiwell.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "welltransl.h"
#include "wellwriter.h"

#include "uibutton.h"
#include "uiconstvel.h"
#include "uifileinput.h"
#include "uifont.h"
#include "uimsg.h"
#include "uiprogressbar.h"
#include "uitable.h"
#include "uid2tmodelgrp.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"


class uiSMWCData
{
public:
			uiSMWCData( const char* wn=0 )
			    : nm_(wn)			{}
    bool		operator ==( const uiSMWCData& wcd ) const
			{ return nm_ == wcd.nm_; }

    BufferString	nm_;
    Coord		coord_;
    float		elev_;
    float		td_;
    float		gl_;
    BufferString	uwi_;
};


#define mDefNrRows 10

uiSimpleMultiWellCreate::uiSimpleMultiWellCreate( uiParent* p )
    : uiDialog(p,Setup(tr("Import Well Locations"),mNoDlgTitle,
		       mODHelpKey(mSimpleMultiWellCreateHelpID) )
		 .savebutton(true).savetext(tr("Display after import"))
		 .modal(false))
    , velfld_(nullptr)
    , zinft_(SI().depthsInFeet())
    , zun_(UnitOfMeasure::surveyDefDepthUnit())
    , overwritepol_(0)
{
    setOkText( uiStrings::sImport() );

    tbl_ = new uiTable( this, uiTable::Setup(mDefNrRows,7).rowgrow(true)
						  .manualresize(true)
						  .selmode(uiTable::Multi),
			"Data Table" );
    tbl_->setFont( FontList().get(FontData::Fixed) );
    tbl_->setColumnLabel( 0, mJoinUiStrs(sWell(),sName().toLower()) );
    const uiString xunstr = SI().getUiXYUnitString();
    tbl_->setColumnLabel( 1,
	toUiString("%1 %2").arg(uiStrings::sXcoordinate()).arg(xunstr) );
    tbl_->setColumnLabel( 2,
	toUiString("%1 %2").arg(uiStrings::sYcoordinate()).arg(xunstr) );
    const uiString zun = UnitOfMeasure::surveyDefDepthUnitAnnot( true, true );
    tbl_->setColumnLabel( 3, tr("[KB %1]").arg(zun) );
    tbl_->setColumnToolTip( 3, Well::Info::sKBElev() );
    tbl_->setColumnLabel( 4, tr("[TD %1]").arg(zun) );
    tbl_->setColumnToolTip( 4, Well::Info::sTD() );
    tbl_->setColumnLabel( 5, tr("[GL %1]").arg(zun) );
    tbl_->setColumnToolTip( 5, Well::Info::sGroundElev() );
    tbl_->setColumnLabel( 6, tr("[UWI]") );
    tbl_->setColumnToolTip( 6, Well::Info::sUwid() );
    tbl_->setColumnResizeMode( uiTable::Interactive );
    tbl_->setSelectionBehavior( uiTable::SelectRows );
    tbl_->setColumnStretchable( 6, true );

    uiPushButton* pb = new uiPushButton( this, tr("Read file"),
	    mCB(this,uiSimpleMultiWellCreate,rdFilePush), false );
    pb->attach( ensureBelow, tbl_ );

    if ( SI().zIsTime() )
    {
	const float defvel = uiD2TModelGroup::getDefaultTemporaryVelocity();
	const uiString vellbl( uiD2TModelGroup::sKeyTemporaryVelUiStr() );
	velfld_ = new uiConstantVel( this, defvel, vellbl );
	velfld_->attach( rightTo, pb );
	velfld_->attach( rightBorder );
    }

    tbl_->setPrefWidth( 600 );
    tbl_->resizeColumnsToContents();

    progbar_ = new uiProgressBar( this, "Creating wells" );
    progbar_->attach( stretchedBelow, pb );
}


class uiSimpleMultiWellCreateReadDataAscio : public Table::AscIO
{
public:
uiSimpleMultiWellCreateReadDataAscio( const Table::FormatDesc& fd,
				      od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{
    atend_ = !getHdrVals( strm_ );
}

bool isXY()
{
    return formOf( false, 1 ) == 0;
}

uiSMWCData* getLine()
{
    if ( atend_ ) return nullptr;

    atend_ = true;
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return nullptr;
    atend_ = false;

    auto* wcd = new uiSMWCData;
    wcd->nm_ = getText( 0 );
    if ( isXY() )
	wcd->coord_ = getPos( 1, 2 );
    else
    {
	LatLong ll;
	ll.setFromString( getText(1), true );
	ll.setFromString( getText(2), false );
	wcd->coord_ = LatLong::transform( ll );
    }

    if ( wcd->nm_.isEmpty()
      || mIsUdf(wcd->coord_.x) || mIsUdf(wcd->coord_.y)
      || (wcd->coord_.x == 0 && wcd->coord_.y == 0) )
    {
	delete wcd;
	return nullptr;
    }

    wcd->elev_ = getFValue( 3 );
    wcd->td_ = getFValue( 4 );
    wcd->gl_ = getFValue( 5 );
    wcd->uwi_ = getText( 6 );
    return wcd;
}

    od_istream&		strm_;

    bool		atend_;

};


class uiSimpleMultiWellCreateReadData : public uiDialog
{ mODTextTranslationClass(uiSimpleMultiWellCreateReadData)
public:

uiSimpleMultiWellCreateReadData( uiSimpleMultiWellCreate& p )
    : uiDialog(&p,uiDialog::Setup(tr("Multi-well creation"),
				  uiStrings::phrCreate(tr("multiple wells")),
			    mODHelpKey(mSimpleMultiWellCreateReadDataHelpID)))
    , dlg_(p)
    , fd_("Simple multi-welldata")
{
    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::Table );

    fd_.bodyinfos_ += new Table::TargetInfo( "Well name", Table::Required );
    fd_.bodyinfos_ += Table::TargetInfo::mkHorPosition( true, false, true );
    Table::TargetInfo* ti = Table::TargetInfo::mkDepthPosition( false );
    ti->setName( Well::Info::sKeyKBElev() );
    fd_.bodyinfos_ += ti;
    ti = Table::TargetInfo::mkDepthPosition( false );
    ti->setName( Well::Info::sKeyTD() );
    fd_.bodyinfos_ += ti;
    ti = Table::TargetInfo::mkDepthPosition( false );
    ti->setName( Well::Info::sKeyGroundElev() );
    fd_.bodyinfos_ += ti;
    fd_.bodyinfos_ += new Table::TargetInfo( Well::Info::sKeyUwid(),
				      Table::Optional );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
				mODHelpKey(mTableImpDataSelwellsHelpID));
    dataselfld_->attach( alignedBelow, inpfld_ );
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool acceptOK( CallBacker* ) override
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(mJoinUiStrs(sInputFile(),sName())) )
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet(uiStrings::phrCannotOpen(uiStrings::sInputFile()))

    if ( !dataselfld_->commit() )
	return false;

    uiSimpleMultiWellCreateReadDataAscio aio( fd_, strm );
    ManagedObjectSet<uiSMWCData> wcds;
    uiSMWCData* wcd = aio.getLine();
    while ( wcd )
    {
	wcds += wcd;
	wcd = aio.getLine();
    }

    dlg_.fillTable( wcds );
    return true;
}

    uiSimpleMultiWellCreate&	dlg_;
    Table::FormatDesc		fd_;

    uiFileInput*		inpfld_;
    uiTableImpDataSel*		dataselfld_;

};


void uiSimpleMultiWellCreate::rdFilePush( CallBacker* )
{
    uiSimpleMultiWellCreateReadData dlg( *this );
    dlg.go();

    tbl_->resizeColumnsToContents();
}


bool uiSimpleMultiWellCreate::wantDisplay() const
{
    return saveButtonChecked();
}


bool uiSimpleMultiWellCreate::createWell( const uiSMWCData& wcd,
					  const IOObj& ioobj )
{
    RefMan<Well::Data> wd = new Well::Data( wcd.nm_ );
    wd->info().surfacecoord_ = wcd.coord_;
    wd->info().uwid_ = wcd.uwi_;
    wd->info().groundelev_ = wcd.gl_;

    Interval<float> drg( -wcd.elev_, wcd.td_-wcd.elev_ );
    wd->track().addPoint( wcd.coord_, drg.start, 0 );
    wd->track().addPoint( wcd.coord_, drg.stop, wcd.td_ );
    if ( velfld_ )
    {
	Well::D2TModel* d2t = new Well::D2TModel("Simple");
	d2t->makeFromTrack(  wd->track(), vel_, wd->info().replvel_ );
	wd->setD2TModel( d2t );
    }

    Well::Writer wr( ioobj, *wd );
    if ( !wr.put() )
    {
	uiString msg = tr( "Cannot write data for '%1':\n%2" )
		     .arg( wcd.nm_ ).arg( wr.errMsg() );
	uiMSG().error( msg );
	return false;
    }

    crwellids_.add( ioobj.key() );
    return true;
}


IOObj* uiSimpleMultiWellCreate::getIOObj( const char* wellnm )
{
    IOObj* ioobj = IOM().getLocal( wellnm, "Well" );
    if ( ioobj )
    {
	if ( overwritepol_ == 0 )
	    overwritepol_ = uiMSG().askGoOn(
		    tr("Do you want to overwrite existing wells?"),
		    true) ? 1 : -1;
	if ( overwritepol_ == -1 )
	    { delete ioobj; return 0; }
	ioobj->implRemove();
    }

    if ( !ioobj )
    {
	CtxtIOObj ctio( mIOObjContext(Well) );
	ctio.setName( wellnm );
	ctio.fillObj();
	ioobj = ctio.ioobj_;
    }
    return ioobj;
}


bool uiSimpleMultiWellCreate::getWellCreateData( int irow, const char* wellnm,
						 uiSMWCData& wcd )
{
    wcd.coord_.x = tbl_->getDValue( RowCol(irow,1) );
    wcd.coord_.y = tbl_->getDValue( RowCol(irow,2) );
    if ( mIsUdf(wcd.coord_.x) || mIsUdf(wcd.coord_.y) )
    {
	uiMSG().message(tr("No full coordinate for %1"
			   "\nWell not created").arg(wellnm));
	return false;
    }

    wcd.elev_ = tbl_->getFValue( RowCol(irow,3) );
    if ( mIsUdf(wcd.elev_) ) wcd.elev_ = 0;
    if ( zinft_ && zun_ ) wcd.elev_ = zun_->internalValue( wcd.elev_ );

    wcd.td_ = tbl_->getFValue( RowCol(irow,4) );
    if ( wcd.td_ > 1e-6 && !mIsUdf(wcd.td_) )
    {
	if ( zinft_ && zun_ ) wcd.td_ = zun_->internalValue( wcd.td_ );
    }
    else
    {
	float survzstop = SI().zRange(false).stop;
	if ( velfld_ )
	    survzstop *= vel_ / 2.f;

	wcd.td_ = survzstop - wcd.elev_;
    }

    wcd.gl_ = tbl_->getFValue( RowCol(irow,5) );
    if ( zinft_ && zun_ && !mIsUdf(wcd.gl_) )
	wcd.gl_ = zun_->internalValue(  wcd.gl_ );

    wcd.uwi_ = tbl_->text( RowCol(irow,6) );
    return true;
}


bool uiSimpleMultiWellCreate::acceptOK( CallBacker* )
{
    crwellids_.erase();
    vel_ = velfld_ ? velfld_->getFValue() : Vel::getGUIDefaultVelocity();
    if ( zinft_ && SI().zIsTime() && zun_ )
	vel_ = zun_->internalValue( vel_ );

    if ( vel_ < 1e-5 || mIsUdf(vel_) )
	{ uiMSG().error(tr("Please enter a valid velocity")); return false; }

    IOM().to( IOObjContext::WllInf );

    progbar_->setTotalSteps( tbl_->nrRows() );
    for ( int irow=0; irow<tbl_->nrRows(); irow++ )
    {
       BufferString wellnm( tbl_->text(RowCol(irow,0) ) );
       if ( wellnm.trimBlanks().isEmpty() )
	   break;

       uiSMWCData wcd( wellnm );
       if ( !getWellCreateData(irow,wellnm,wcd) )
	   continue;

       IOObj* ioobj = getIOObj( wellnm );
       if ( !ioobj )
	   continue;

       progbar_->setProgress( irow + 1 );
       if ( !createWell(wcd,*ioobj) )
	   return false;
    }

    progbar_->setProgress( 0 );
    if ( crwellids_.isEmpty() )
    {
	return !uiMSG().askGoOn( tr("No wells have been imported."
			     "\n\nDo you want to make changes to the table?"),
			     uiStrings::sYes(), tr("No, Quit") );
    }

    tbl_->clearTable();
    return true;
}


void uiSimpleMultiWellCreate::addRow( const uiSMWCData& wcd, int& prevrow )
{
}


void uiSimpleMultiWellCreate::fillTable( const ObjectSet<uiSMWCData>& wcds )
{
    if ( wcds.size() > mDefNrRows )
	tbl_->setNrRows( wcds.size() );

    for ( int idx=0; idx<wcds.size(); idx++ )
	fillRow( idx, *wcds.get(idx) );

    tbl_->resizeColumnsToContents();
}


void uiSimpleMultiWellCreate::fillRow( int row, const uiSMWCData& wcd )
{
    RowCol rc( row, 0 );
    tbl_->setText( rc, wcd.nm_ ); rc.col()++;
    tbl_->setValue( rc, wcd.coord_.x ); rc.col()++;
    tbl_->setValue( rc, wcd.coord_.y ); rc.col()++;
    float v = wcd.elev_; if ( zinft_ && zun_ ) v = zun_->userValue( v );
    tbl_->setValue( rc, v ); rc.col()++;
    v = wcd.td_; if ( zinft_ && zun_ ) v = zun_->userValue( v );
    tbl_->setValue( rc, v ); rc.col()++;
    v = wcd.gl_; if ( !mIsUdf(v) && zinft_ && zun_ ) v = zun_->userValue( v );
    tbl_->setValue( rc, v ); rc.col()++;
    tbl_->setText( rc, wcd.uwi_ );
}
