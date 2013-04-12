/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "gmtclip.h"

#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "pickset.h"
#include "picksettr.h"
#include "strmdata.h"
#include "strmprov.h"


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


bool GMTClip::execute( std::ostream& strm, const char* fnm )
{
    bool isstartofclipping = false;
    getYN( ODGMT::sKeyStartClipping(), isstartofclipping );
    BufferString comm( "psclip " );
    if ( !isstartofclipping )
    {
	strm << "Terminating clipping ... ";
	comm += "-C -O -K";
	comm += " 1>> "; comm += fileName( fnm );
	if ( !execCmd(comm,strm) )
	    mErrStrmRet("Failed")

	strm << "Done" << std::endl;
	return true;
    }

    MultiID id;
    get( sKey::ID(), id );
    const IOObj* setobj = IOM().get( id );
    if ( !setobj ) mErrStrmRet("Cannot find polygon")

    strm << "Activating clipping with polygon " << setobj->name() << " ...  ";
    Pick::Set ps;
    BufferString errmsg;
    if ( !PickSetTranslator::retrieve(ps,setobj,true,errmsg) )
	mErrStrmRet( errmsg )

    bool clipoutside = false;
    getYN( ODGMT::sKeyClipOutside(), clipoutside );
    BufferString rangestr; mGetRangeProjString( rangestr, "X" );
    comm += rangestr;
    if ( !clipoutside ) comm += " -N";
    comm += " -O -K 1>> "; comm += fileName( fnm );
    StreamData sd = makeOStream( comm, strm );
    if ( !sd.usable() )
	mErrStrmRet("Failed")

    for ( int idx=0; idx<ps.size(); idx++ )
	*sd.ostrm << ps[idx].pos_.x << " " << ps[idx].pos_.y << std::endl;

    sd.close();
    strm << "Done" << std::endl;
    return true;
}

