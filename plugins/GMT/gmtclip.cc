/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
________________________________________________________________________

-*/


#include "gmtclip.h"

#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "picksetmanager.h"
#include "strmdata.h"
#include "od_ostream.h"


int GMTClip::factoryid_ = -1;

void GMTClip::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Clipping", GMTClip::createInstance );
}

GMTPar* GMTClip::createInstance( const IOPar& iop )
{
    return new GMTClip( iop );
}


bool GMTClip::isStart() const
{
    bool ret = false;
    getYN( ODGMT::sKeyStartClipping(), ret );
    return ret;
}


const char* GMTClip::userRef() const
{
    BufferString* str = new BufferString( "Clipping " );
    bool isstartofclipping = false, clipoutside = false;
    getYN( ODGMT::sKeyStartClipping(), isstartofclipping );
    getYN( ODGMT::sKeyClipOutside(), clipoutside );
    *str += isstartofclipping ? "Start: " : "Stop";
    if ( isstartofclipping ) *str += clipoutside ? "Outside" : "Inside";
    return str->buf();
}


bool GMTClip::fillLegendPar( IOPar& par ) const
{
    return false;
}


bool GMTClip::execute( od_ostream& strm, const char* fnm )
{
    bool isstartofclipping = false;
    getYN( ODGMT::sKeyStartClipping(), isstartofclipping );
    BufferString comm( "@psclip " );
    if ( !isstartofclipping )
    {
	strm << "Terminating clipping ... ";
	comm += "-C -O -K";
	comm += " 1>> "; comm += fileName( fnm );
	if ( !execCmd(comm,strm) )
	    mErrStrmRet("Failed")

	strm << "Done" << od_endl;
	return true;
    }

    DBKey id; get( sKey::ID(), id );
    uiRetVal uirv = uiRetVal::OK();
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( id, uirv );
    if ( !ps )
	mErrStrmRet(uirv.getText())

    strm << "Activating clipping with polygon " << ps->name() << " ...  ";
    bool clipoutside = false;
    getYN( ODGMT::sKeyClipOutside(), clipoutside );
    BufferString rangestr; mGetRangeProjString( rangestr, "X" );
    comm += rangestr;
    if ( !clipoutside ) comm += " -N";
    comm += " -O -K 1>> "; comm += fileName( fnm );
    od_ostream procstrm = makeOStream( comm, strm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed")

    Pick::SetIter psiter( *ps );
    while ( psiter.next() )
    {
	const Coord pos = psiter.getPos();
	procstrm << pos.x << " " << pos.y << "\n";
    }

    strm << "Done" << od_endl;
    return true;
}
