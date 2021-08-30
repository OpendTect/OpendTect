/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2010
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
#include "uimsg.h"
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

    tbl_ = new uiTable( this, uiTable::Setup(10,7).rowgrow(true)
						  .manualresize(true)
						  .selmode(uiTable::Multi),
			"Data Table" );
    tbl_->setColumnLabel( 0, mJoinUiStrs(sWell(),sName().toLower()) );
    const uiString xunstr = SI().getUiXYUnitString();
    tbl_->setColumnLabel( 1,
		toUiString("%1 %2").arg(uiStrings::sX()).arg(xunstr) );
    tbl_->setColumnLabel( 2,
		toUiString("%1 %2").arg(uiStrings::sY()).arg(xunstr) );
    const uiString zun = UnitOfMeasure::surveyDefDepthUnitAnnot( true, true );
    tbl_->setColumnLabel( 3, tr("[KB %1]").arg(zun) );
    tbl_->setColumnToolTip( 3, Well::Info::sKBElev() );
    tbl_->setColumnLabel( 4, tr("[TD %1]").arg(zun) );
    tbl_->setColumnToolTip( 4, Well::Info::sTD() );
    tbl_->setColumnLabel( 5, tr("[GL %1]").arg(zun) );
    tbl_->setColumnToolTip( 5, Well::Info::sGroundElev() );
    tbl_->setColumnLabel( 6, tr("[UWI]") );
    tbl_->setColumnToolTip( 6, Well::Info::sUwid() );
    tbl_->setColumnResizeMode( uiTable::ResizeToContents );
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

bool getLine()
{
    if ( atend_ ) return false;

    atend_ = true;
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;
    atend_ = false;

    wcd_.nm_ = getText( 0 );
    if ( isXY() )
	wcd_.coord_ = getPos( 1, 2 );
    else
    {
	LatLong ll;
	ll.setFromString( getText(1), true );
	ll.setFromString( getText(2), false );
	wcd_.coord_ = LatLong::transform( ll );
    }

    if ( wcd_.nm_.isEmpty()
      || mIsUdf(wcd_.coord_.x) || mIsUdf(wcd_.coord_.y)
      || (wcd_.coord_.x == 0 && wcd_.coord_.y == 0) )
	return false;

    wcd_.elev_ = getFValue( 3 );
    wcd_.td_ = getFValue( 4 );
    wcd_.gl_ = getFValue( 5 );
    wcd_.uwi_ = getText( 6 );
    return true;
}

    od_istream&		strm_;

    bool		atend_;
    uiSMWCData		wcd_;

};


class uiSimpleMultiWellCreateReadData : public uiDialog
{ mODTextTranslationClass(uiSimpleMultiWellCreateReadData)
public:

uiSimpleMultiWellCreateReadData( uiSimpleMultiWellCreate& p )
    : uiDialog(&p,uiDialog::Setup(tr("Multi-well creation"),
				  uiStrings::phrCreate(tr("multiple wells")),
			    mODHelpKey(mSimpleMultiWellCreateReadDataHelpID)))
    , par_(p)
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

bool acceptOK( CallBacker* )
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
    int prevrow = -1;
    while ( aio.getLine() )
	par_.addRow( aio.wcd_, prevrow );

    return true;
}

    uiSimpleMultiWellCreate&	par_;
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
    wd->info().surfacecoord = wcd.coord_;
    wd->info().uwid = wcd.uwi_;
    wd->info().groundelev = wcd.gl_;

    Interval<float> drg( -wcd.elev_, wcd.td_-wcd.elev_ );
    wd->track().addPoint( wcd.coord_, drg.start, 0 );
    wd->track().addPoint( wcd.coord_, drg.stop, wcd.td_ );
    if ( velfld_ )
    {
	Well::D2TModel* d2t = new Well::D2TModel("Simple");
	d2t->makeFromTrack(  wd->track(), vel_, wd->info().replvel );
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


class SimpleMultiWellImporter : public Executor
{
public:
SimpleMultiWellImporter( uiSimpleMultiWellCreate& mygui )
    : Executor("Multi-well creator")
    , mygui_(mygui)
{
}

od_int64 nrDone() const
{
    return curidx_;
}

od_int64 totalNr() const
{
    return mygui_.tbl_->nrRows();
}

uiString uiNrDoneText() const
{
    return uiStrings::sPositionsDone();
}

int nextStep()
{
    if ( curidx_ >= totalNr() )
	return Finished();

    BufferString wellnm( mygui_.tbl_->text(RowCol(curidx_,0) ) );
    if ( wellnm.trimBlanks().isEmpty() )
	return Finished();

    uiSMWCData wcd( wellnm );
    if ( !mygui_.getWellCreateData(curidx_++,wellnm,wcd) )
	return MoreToDo();

    PtrMan<IOObj> ioobj = mygui_.getIOObj( wellnm );
    if ( !ioobj )
	return MoreToDo();

    if ( !mygui_.createWell(wcd,*ioobj) )
	return ErrorOccurred();

    return MoreToDo();
}

	int				curidx_ = 0;
	uiSimpleMultiWellCreate&	mygui_;
};


bool uiSimpleMultiWellCreate::acceptOK( CallBacker* )
{
    crwellids_.erase();
    vel_ = velfld_ ? velfld_->getFValue() : Vel::getGUIDefaultVelocity();
    if ( zinft_ && SI().zIsTime() && zun_ )
	vel_ = zun_->internalValue( vel_ );

    if ( vel_ < 1e-5 || mIsUdf(vel_) )
	{ uiMSG().error(tr("Please enter a valid velocity")); return false; }

    IOM().to( WellTranslatorGroup::ioContext().getSelKey() );

    SimpleMultiWellImporter impexec( *this );
    uiTaskRunner uitr( this );
    if ( !uitr.execute(impexec) )
	return false;

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
    if ( prevrow < 0 )
    {
	for ( int irow=0; irow<tbl_->nrRows(); irow++ )
	{
	    const BufferString wnm( tbl_->text(RowCol(irow,0)) );
	    if ( wnm.isEmpty() )
		break;
	    prevrow = irow;
	}
    }

    prevrow++;
    RowCol rc( prevrow, 0 );
    if ( rc.row() >= tbl_->nrRows() )
	tbl_->setNrRows( tbl_->nrRows()+10 );

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
