/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: gmtarray2dinterpol.cc,v 1.1 2010-08-13 11:03:33 cvsnageswara Exp $";

#include "gmtarray2dinterpol.h"

#include "arraynd.h"
#include "string2.h"
#include "strmdata.h"
#include "strmprov.h"
#include "oddirs.h"

#include "file.h"
#include "filepath.h"

#include <iostream>

static const char* sKeyGMTUdf = "NaN";

const char* GMTArray2DInterpol::sType()
{ return "Continuous curvature(GMT)"; }


void GMTArray2DInterpol::initClass()
{
    Array2DInterpol::factory().addCreator( create, sType() );
}


Array2DInterpol* GMTArray2DInterpol::create()
{
    return new GMTArray2DInterpol;
}


GMTArray2DInterpol::GMTArray2DInterpol()
    : cmd_("@surface -I1 ")
    , msg_("Continuous curvature")
    , tmpfnm_("")
    , nrdone_(0)
{}


void GMTArray2DInterpol::setPar( IOPar& iop )
{
    iopar_  = iop;
}


od_int64 GMTArray2DInterpol::nrIterations() const
{
    return nrrows_ + 1;
}


const char* GMTArray2DInterpol::message() const
{
    return msg_.buf();
}


bool GMTArray2DInterpol::mkCommand()
{
    if ( iopar_.isEmpty() )
    {
	msg_ = "Tension parameter missing";
	return false;
    }

    float tension;
    iopar_.get( "Tension", tension );
    tmpfnm_ = FilePath::getTempName( "grd" );
    cmd_.add( "-T" ).add( tension )
	.add( " -G" ).add( tmpfnm_ )
	.add( " -R0/" ).add( nrrows_ - 1 ).add( "/0/" ).add( nrcols_ - 1 );

    return true;
}


bool GMTArray2DInterpol::doPrepare( int nrthreads )
{
    if ( filltype_ != Array2DInterpol::Full )
    {
	msg_ = "Selected algorithm will work only on full survey";
	return false;
    }

    if ( !mkCommand() )
	return false;

    sd_ = StreamProvider( cmd_ ).makeOStream();
    if ( !sd_.usable() )
	return false;

    return true;
}


bool GMTArray2DInterpol::doWork( od_int64 start, od_int64 stop, int threadid )
{
    nrdone_ = 0;
    for ( int ridx=start; ridx<stop; ridx++ )
    {
	if ( !*sd_.ostrm )
	    break;

	for ( int cidx=0; cidx<nrcols_; cidx++ )
	{
	    if ( !*sd_.ostrm )
		break;

	    if ( !arr_->info().validPos(ridx, cidx) )
		continue;

	    if ( mIsUdf(arr_->get(ridx,cidx)) )
		continue;

	    *sd_.ostrm << ridx << " " << cidx << " " << arr_->get(ridx,cidx)
						     << std::endl;
	}

	nrdone_++;
    }

    sd_.close();

    return true;
}


bool GMTArray2DInterpol::doFinish( bool success )
{
    cmd_ = "@grd2xyz ";
    cmd_.add( tmpfnm_ );

    sd_ = StreamProvider( cmd_ ).makeIStream( true, false );
    if ( !sd_.usable() )
	return false;

    nrdone_ = 0;
    for ( int ridx=0; ridx<nrrows_; ridx++ )
    {

	if ( !*sd_.istrm )
	    break;

	for ( int cidx=0; cidx<nrcols_; cidx++ )
	{
	    if ( !*sd_.istrm )
		break;

	    if ( !arr_->info().validPos(ridx, cidx) )
		continue;

	    char rowstr[10], colstr[10], valstr[20];
	    *sd_.istrm >> rowstr >> colstr >> valstr;
	    if ( !strcmp(rowstr,sKeyGMTUdf) || !strcmp(colstr,sKeyGMTUdf)
	      				    || !strcmp(valstr,sKeyGMTUdf) )
		continue;

	    int row, col; float val;
	    if ( !getFromString(row, rowstr) || !getFromString(col, colstr)
					     || !getFromString(val, valstr) )
		continue;

	    arr_->set( row, col, val );
	}

	nrdone_++;
    }

    sd_.close();
    if ( nrdone_ == 0 )
    {
	msg_ = "Invalid positinos found";
	return false;
    }

    File::remove( tmpfnm_ );

    return true;
}
