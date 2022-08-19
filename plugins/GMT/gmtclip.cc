/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "gmtclip.h"

#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "pickset.h"
#include "picksettr.h"
#include "od_ostream.h"


int GMTClip::factoryid_ = -1;

void GMTClip::initClass()
{
    if ( factoryid_ < 1 )
	factoryid_ = GMTPF().add( "Clipping", GMTClip::createInstance );
}

GMTPar* GMTClip::createInstance( const IOPar& iop, const char* workdir )
{
    return new GMTClip( iop, workdir );
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


bool GMTClip::fillLegendPar( IOPar& ) const
{
    return false;
}


bool GMTClip::doExecute( od_ostream& strm, const char* fnm )
{
    bool isstartofclipping = false;
    getYN( ODGMT::sKeyStartClipping(), isstartofclipping );
    OS::MachineCommand clipmc( "psclip" );
    if ( !isstartofclipping )
    {
	strm << "Terminating clipping ... ";
	clipmc.addArg( "-O" ).addArg( "-K" ).addArg( "-C" );
	if ( !execCmd(clipmc,strm,fnm) )
	    mErrStrmRet("Failed")

	strm << "Done" << od_endl;
	return true;
    }

    MultiID id;
    get( sKey::ID(), id );
    const IOObj* setobj = IOM().get( id );
    if ( !setobj ) mErrStrmRet("Cannot find polygon")

    strm << "Activating clipping with polygon " << setobj->name() << " ...  ";
    BufferString errmsg;
    RefMan<Pick::Set> ps = new Pick::Set;
    if ( !PickSetTranslator::retrieve(*ps,setobj,true,errmsg) )
	mErrStrmRet( errmsg )

    bool clipoutside = false;
    getYN( ODGMT::sKeyClipOutside(), clipoutside );
    BufferString mapprojstr, rgstr;
    mGetRangeString(rgstr)
    mGetProjString(mapprojstr,"X")
    clipmc.addArg( mapprojstr ).addArg( rgstr );
    clipmc.addArg( "-O" ).addArg( "-K" );
    if ( !clipoutside )
	clipmc.addArg( "-N" );
    od_ostream procstrm = makeOStream( clipmc, strm, fnm );
    if ( !procstrm.isOK() )
	mErrStrmRet("Failed")

    for ( int idx=0; idx<ps->size(); idx++ )
    {
	const Coord3& pos = ps->getPos( idx );
	procstrm << pos.x << " " << pos.y << "\n";
    }

    strm << "Done" << od_endl;
    return true;
}
