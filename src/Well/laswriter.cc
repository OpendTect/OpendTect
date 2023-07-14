/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "laswriter.h"

#include "dateinfo.h"
#include "od_ostream.h"
#include "string2.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"

static const int cNrMDDecimals = 4;
static const int cNrXYDecimals = 3;
static const int cNrValDecimals = 4;

LASWriter::LASWriter( const Well::Data& wd, const BufferStringSet& lognms,
		      const char* lasfnm )
    : Executor("")
    , wd_(&wd)
    , logs_(*new Well::LogSet)
    , lognms_(lognms)
    , lasfnm_(lasfnm)
{
    nullvalue_ = "-999.2500";

    for ( int idx=0; idx<lognms_.size(); idx++ )
    {
	const Well::Log* log = wd_->logs().getLog( lognms_.get(idx).buf() );
	logs_.add( new Well::Log(*log) );
    }

    mdrg_ = logs_.dahInterval();
    mdrg_.step = 0.1524f;
}


LASWriter::~LASWriter()
{
    delete &logs_;
}


void LASWriter::setMDRange( const StepInterval<float>& rg )
{
    mdrg_ = rg;
}


void LASWriter::setNullValue( const char* nv )
{
    nullvalue_ = nv;
}


int LASWriter::nextStep()
{
    od_ostream strm( lasfnm_.buf() );
    if ( !strm.isOK() )
	return ErrorOccurred();

    writeVersionInfoSection( strm );
    writeWellInfoSection( strm );
    writeCurveInfoSection( strm );
    writeParameterInfoSection( strm );
    writeOtherSection( strm );
    writeLogData( strm );
    strm << od_newline;
    nrdone_++;
    return Finished();
}


bool LASWriter::writeVersionInfoSection( od_ostream& strm )
{
    strm << "~Version Information Section\n";
    strm << "VERS.              2.0  : CWLS LOG ASCII STANDARD - VERSION 2.0\n";
    strm << "WRAP.              NO   : ONE LINE PER DEPTH STEP\n";

    BufferString datestr;
    DateInfo di;
    di.toString( datestr );
    strm << "CREA.              " << datestr.buf() << "\n";
    strm << "#\n";
    return true;
}


static const int cValStart = 20;
static const int cDescStart = 40;

static BufferString create( const char* mnem, const char* unit, const char* val,
			    const char* desc, bool newline=true )
{
    BufferString str( mnem, ".", unit );
    int sz = str.size();
    str.addSpace( cValStart-sz-1 );
    str.add( val );
    sz = str.size();
    str.addSpace( cDescStart-sz-1 );
    str.add( ": " ).add( desc );
    if ( newline )
	str.addNewLine();
    return str;
}


bool LASWriter::writeWellInfoSection( od_ostream& strm )
{
    BufferString zstart, zstop, zstep, xcoord, ycoord;
    zstart.set( mdrg_.start, cNrMDDecimals );
    zstop.set( mdrg_.stop, cNrMDDecimals );
    zstep.set( mdrg_.step, cNrMDDecimals );
    xcoord.set( wd_->info().surfacecoord_.x, cNrXYDecimals );
    ycoord.set( wd_->info().surfacecoord_.y, cNrXYDecimals );
    const char* depthunit = zinfeet_ ? "F" : "M";
    const char* xyunit = SI().xyInFeet() ? "F" : "M";

    strm << "~Well Information Section\n";
    strm << "#MNEM.UNIT         VALUE/NAME          DESCRIPTION\n";
    strm << "#_________         __________          ___________\n";
    strm << create( "STRT", depthunit, zstart, "START DEPTH" ).buf();
    strm << create( "STOP", depthunit, zstop, "STOP DEPTH" ).buf();
    strm << create( "STEP", depthunit, zstep, "STEP" ).buf();
    strm << create( "NULL", "", "-999.2500", "NULL VALUE" ).buf();
    strm << create( "COMP", "", "", "COMPANY" ).buf();
    strm << create( "WELL", "", wd_->name(), "WELL NAME" ).buf();
    strm << create( "FLD", "", wd_->info().field_, "FIELD" ).buf();
    strm << create( "LOC", "", "", "LOCATION" ).buf();
    strm << create( "PROV", "", wd_->info().province_, "PROVINCE" ).buf();
    strm << create( "CNTY", "", wd_->info().county_, "COUNTY" ).buf();
    strm << create( "STAT", "", wd_->info().state_, "STATE" ).buf();
    strm << create( "CTRY", "", wd_->info().country_, "COUNTRY" ).buf();
    strm << create( "SRVC", "", wd_->info().oper_, "SERVICE COMPANY" ).buf();
    strm << create( "DATE", "", "", "LOG DATE" ).buf();
    strm << create( "UWI", "", wd_->info().uwid_, "UNIQUE WELL ID" ).buf();
    strm << create( "XCOORD", xyunit, xcoord, "SURFACE X" ).buf();
    strm << create( "YCOORD", xyunit, ycoord, "SURFACE Y" ).buf();
    strm << "#\n";
    return true;
}


bool LASWriter::writeCurveInfoSection( od_ostream& strm )
{
    const char* depthunit = zinfeet_ ? "F" : "M";

    strm << "~Curve Information Section\n";
    strm << "#MNEM.UNIT         API CODE            DESCRIPTION\n";
    strm << "#_________         ________            ___________\n";
    strm << create( "DEPT", depthunit, "", "0 Depth" );
    for ( int idx=0; idx<logs_.size(); idx++ )
    {
	const Well::Log& log = logs_.getLog( idx );
	BufferString uomstr = log.unitMeasLabel();
	uomstr.remove( ' ' );
	if ( writelognm_ )
	    strm << create( log.name(), uomstr, "",
			BufferString(toString(idx+1)," ",log.mnemonicLabel()) );
	else
	    strm << create( log.mnemonicLabel(), uomstr, "",
			BufferString(toString(idx+1)," ",log.name()) );
    }

    strm << "#\n";
    return true;
}


bool LASWriter::writeParameterInfoSection( od_ostream& strm )
{
// optional
    BufferString kbstr, glstr;
    kbstr.set( wd_->track().getKbElev(), cNrMDDecimals );
    if ( mIsUdf(wd_->info().groundelev_) )
	glstr.set( nullvalue_ );
    else
	glstr.set( wd_->info().groundelev_, cNrMDDecimals );

    const char* depthunit = zinfeet_ ? "F" : "M";
    strm << "~Parameter Information Section\n";
    strm << "#MNEM.UNIT           VALUE             DESCRIPTION\n";
    strm << "#_________         __________          ___________\n";
    strm << create( "EKB", depthunit, kbstr, "Elevation Kelly Bushing" ).buf();
    strm << create( "EGL", depthunit, glstr, "Elevation Ground Level" ).buf();
    strm << "#\n";
    return true;
}


bool LASWriter::writeOtherSection( od_ostream& strm )
{
// optional
    strm << "~Other Information Section\n";
    strm << "#\n";
    return true;
}


bool LASWriter::writeLogData( od_ostream& strm )
{
    const BufferString mdformatstr = cformat( 'f', 12, cNrMDDecimals );
    const BufferString valformatstr =
			cformat( 'f', columnwidth_, cNrValDecimals );
    BufferString word( columnwidth_+1, false );
    const int nrz = mdrg_.nrSteps() + 1;

    const UnitOfMeasure* mdunit =
		UnitOfMeasure::getGuessed( zinfeet_ ? "ft" : "m" );
    const UnitOfMeasure* storunit = Well::Man::surveyDepthStorageUnit();

    strm << "~Ascii Data Section\n";
    for ( int idz=0; idz<nrz; idz++ )
    {
	float md = mdrg_.atIndex( idz );
	od_sprintf( word.getCStr(), word.bufSize(), mdformatstr.buf(),
		    double(md) );
	strm << word;

	md = getConvertedValue( md, mdunit, storunit );
	for ( int idx=0; idx<logs_.size(); idx++ )
	{
	    float val = logs_.getLog(idx).getValue( md );
	    if ( mIsUdf(val) )
		val = toFloat( nullvalue_ );

	    od_sprintf( word.getCStr(), word.bufSize(), valformatstr.buf(),
			double(val) );
	    strm << word;
	}
	strm << od_newline;
    }

    return true;
}
