/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uisimplemultiwell.cc,v 1.16 2012-07-12 07:46:34 cvsbert Exp $";


#include "uisimplemultiwell.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "welltrack.h"
#include "welltransl.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"


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
    float		srd_;
    BufferString	uwi_;
};



uiSimpleMultiWellCreate::uiSimpleMultiWellCreate( uiParent* p )
    : uiDialog( p, Setup("Simple Multi-Well Creation",mNoDlgTitle,"107.0.7")
	    		.savebutton(true).savetext("Display after creation") )
    , velfld_(0)
    , zinft_(SI().depthsInFeetByDefault())
    , overwritepol_(0)
{
    tbl_ = new uiTable( this, uiTable::Setup(20,7).rowgrow(true)
	    		.colgrow(false).removecolallowed(false)
			.manualresize( true ), "Data Table" );
    tbl_->setColumnLabel( 0, "Well name" );
    const char* xunstr = SI().getXYUnitString();
    tbl_->setColumnLabel( 1, BufferString("[X",xunstr,"]") );
    tbl_->setColumnLabel( 2, BufferString("[Y",xunstr,"]") );
    const char* zunstr = zinft_ ? " (ft" : " (m";
    tbl_->setColumnLabel( 3, BufferString("[KB Elevation",zunstr,")]") );
    tbl_->setColumnLabel( 4, BufferString("[TD",zunstr,")]") );
    tbl_->setColumnLabel( 5, BufferString("[SRD",zunstr,")]") );
    tbl_->setColumnLabel( 6, "[UWI]" );

    uiPushButton* pb = new uiPushButton( this, "Read file",
	    mCB(this,uiSimpleMultiWellCreate,rdFilePush), false );
    pb->attach( ensureBelow, tbl_ );

    if ( SI().zIsTime() )
    {
	const float defvel = zinft_ ? 8000 : 2000;
	velfld_ = new uiGenInput( this, BufferString("Velocity",zunstr,"/s)"),
		   		  FloatInpSpec(defvel) );
	velfld_->attach( rightTo, pb );
	velfld_->attach( rightBorder );
    }

    tbl_->setPrefWidth( 750 );
    tbl_->setPrefHeight( 500 );
}


class uiSimpleMultiWellCreateReadDataAscio : public Table::AscIO
{
public:
uiSimpleMultiWellCreateReadDataAscio( const Table::FormatDesc& fd,
				      std::istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{
    atend_ = !getHdrVals( strm_ );
}

bool getLine()
{
    if ( atend_ ) return false;

    atend_ = true;
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;
    atend_ = false;

    wcd_.nm_ = text( 0 );
    wcd_.coord_.x = getdValue( 1 );
    wcd_.coord_.y = getdValue( 2 );
    if ( wcd_.nm_.isEmpty()
      || mIsUdf(wcd_.coord_.x) || mIsUdf(wcd_.coord_.y)
      || (wcd_.coord_.x == 0 && wcd_.coord_.y == 0) )
	return false;

    wcd_.elev_ = getfValue( 3 );
    wcd_.td_ = getfValue( 4 );
    wcd_.srd_ = getfValue( 5 );
    wcd_.uwi_ = text( 6 );
    return true;
}

    std::istream&		strm_;

    bool		atend_;
    uiSMWCData		wcd_;

};


class uiSimpleMultiWellCreateReadData : public uiDialog
{
public:

uiSimpleMultiWellCreateReadData( uiSimpleMultiWellCreate& p )
    : uiDialog(&p,uiDialog::Setup("Multi-well creation","Create multiple wells",
			 	 "107.0.8"))
    , par_(p)
    , fd_("Simple multi-welldata")
{
    inpfld_ = new uiFileInput( this, "Input file", uiFileInput::Setup()
		      .withexamine(true).examstyle(uiFileInput::Setup::Table) );

    fd_.bodyinfos_ += new Table::TargetInfo( "Well name", Table::Required );
    fd_.bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    Table::TargetInfo* ti = Table::TargetInfo::mkDepthPosition( false );
    ti->setName( "KB Elevation" ); fd_.bodyinfos_ += ti;
    ti = Table::TargetInfo::mkDepthPosition( false );
    ti->setName( "TD" ); fd_.bodyinfos_ += ti;
    ti = Table::TargetInfo::mkDepthPosition( false );
    ti->setName( "Difference between MSL and SRD" ); fd_.bodyinfos_ += ti;
    fd_.bodyinfos_ += new Table::TargetInfo( "Well ID (UWI)", Table::Optional );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "107.0.9" );
    dataselfld_->attach( alignedBelow, inpfld_ );
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter the input file name" )
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    if ( !dataselfld_->commit() )
	return false;

    uiSimpleMultiWellCreateReadDataAscio aio( fd_, *sd.istrm );
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
}


bool uiSimpleMultiWellCreate::wantDisplay() const
{
    return saveButtonChecked();
}


bool uiSimpleMultiWellCreate::createWell( const uiSMWCData& wcd,
					  const IOObj& ioobj )
{
    Well::Data wd( wcd.nm_ );
    wd.info().surfacecoord = wcd.coord_;
    wd.info().uwid = wcd.uwi_;
    wd.info().surfaceelev = -wcd.srd_;
    Interval<float> drg( -wcd.elev_, wcd.td_-wcd.elev_ );
    wd.track().addPoint( wcd.coord_, drg.start, 0 );
    wd.track().addPoint( wcd.coord_, drg.stop, wcd.td_ );
    if ( velfld_ )
    {
	Well::D2TModel* d2t = new Well::D2TModel("Simple");
	Interval<float> zrg( -wcd.elev_+wcd.srd_, wcd.td_-wcd.elev_+wcd.srd_ );
	Interval<float> trg( zrg ); trg.scale( 1. / vel_ );
	d2t->add( 0, trg.start );
	d2t->add( wcd.td_, trg.stop );
	wd.setD2TModel( d2t );
    }

    PtrMan<Translator> t = ioobj.getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) return true; // Huh?
    if ( !wtr->write(wd,ioobj) )
    {
	BufferString msg( "Cannot write data for '" );
	msg.add( wcd.nm_ ).add( "to file:\n" )
	    .add( ioobj.fullUserExpr(false) );
	uiMSG().error( msg );
	return false;
    }

    crwellids_.add( ioobj.key() );
    return true;
}


IOObj* uiSimpleMultiWellCreate::getIOObj( const char* wellnm )
{
    IOObj* ioobj = IOM().getLocal( wellnm );
    if ( ioobj )
    {
	if ( overwritepol_ == 0 )
	    overwritepol_ = uiMSG().askGoOn(
		    "Do you want to overwrite existing wells?",true) ? 1 : -1;
	if ( overwritepol_ == -1 )
	    { delete ioobj; return 0; }
	ioobj->implRemove();
    }

    if ( !ioobj )
    {
	CtxtIOObj ctio( mIOObjContext(Well) );
	ctio.setName( wellnm );
	ctio.fillObj();
	ioobj = ctio.ioobj;
    }
    return ioobj;
}


bool uiSimpleMultiWellCreate::getWellCreateData( int irow, const char* wellnm,
						 uiSMWCData& wcd )
{
    wcd.coord_.x = tbl_->getdValue( RowCol(irow,1) );
    wcd.coord_.y = tbl_->getdValue( RowCol(irow,2) );
    if ( mIsUdf(wcd.coord_.x) || mIsUdf(wcd.coord_.y) )
    {
	uiMSG().message(BufferString("No full coordinate for ", wellnm,
		    		     "\nWell not created") );
	return false;
    }

    wcd.elev_ = tbl_->getfValue( RowCol(irow,3) );
    if ( mIsUdf(wcd.elev_) ) wcd.elev_ = 0;
    if ( zinft_ ) wcd.elev_ *= mFromFeetFactor;

    wcd.td_ = tbl_->getfValue( RowCol(irow,4) );
    if ( wcd.td_ > 1e-6 && !mIsUdf(wcd.td_) )
    {
	if ( zinft_ ) wcd.td_ *= mFromFeetFactor;
    }
    else
    {
	float survzstop = SI().zRange(false).stop;
	if ( velfld_ )
	    survzstop *= vel_;
	wcd.td_ = survzstop - wcd.elev_;
    }

    wcd.srd_ = tbl_->getfValue( RowCol(irow,5) );
    if ( mIsUdf(wcd.srd_) ) wcd.srd_ = 0;
    if ( zinft_ ) wcd.srd_ *= mFromFeetFactor;

    wcd.uwi_ = tbl_->text( RowCol(irow,6) );
    return true;
}


bool uiSimpleMultiWellCreate::acceptOK( CallBacker* )
{
    crwellids_.erase();
    vel_ = velfld_ ? velfld_->getfValue() : 1000;
    if ( vel_ < 1e-5 || mIsUdf(vel_) )
	{ uiMSG().error("Please enter a valid velocity"); return false; }
    if ( zinft_ )
	vel_ *= mFromFeetFactor;
    vel_ /= 2; // TWT

    IOM().to( WellTranslatorGroup::ioContext().getSelKey() );

    for ( int irow=0; ; irow++ )
    {
	BufferString txtinp( tbl_->text(RowCol(irow,0) ) );
	char* wellnm = txtinp.buf(); mTrimBlanks(wellnm);
	if ( !*wellnm ) break;

	uiSMWCData wcd( wellnm );
	if ( !getWellCreateData(irow,wellnm,wcd) )
	    continue;

	IOObj* ioobj = getIOObj( wellnm );
	if ( !ioobj )
	    continue;

	if ( !createWell(wcd,*ioobj) )
	    return false;
    }

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
    if ( rc.row >= tbl_->nrRows() )
	tbl_->setNrRows( tbl_->nrRows()+10 );

    tbl_->setText( rc, wcd.nm_ ); rc.col++;
    tbl_->setValue( rc, wcd.coord_.x ); rc.col++;
    tbl_->setValue( rc, wcd.coord_.y ); rc.col++;
    float v = wcd.elev_; if ( zinft_ ) v *= mToFeetFactor;
    tbl_->setValue( rc, v ); rc.col++;
    v = wcd.td_; if ( zinft_ ) v *= mToFeetFactor;
    tbl_->setValue( rc, v ); rc.col++;
    v = wcd.srd_; if ( zinft_ ) v *= mToFeetFactor;
    tbl_->setValue( rc, v ); rc.col++;
    tbl_->setText( rc, wcd.uwi_ );
}

