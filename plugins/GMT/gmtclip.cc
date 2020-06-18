/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
________________________________________________________________________

-*/


#include "gmtclip.h"

#include "ioobj.h"
#include "keystrs.h"
#include "picksetmanager.h"
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

    DBKey id; get( sKey::ID(), id );
    uiRetVal uirv;
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( id, uirv );
    if ( !ps )
	mErrStrmRet(uirv.getText())

    strm << "Activating clipping with polygon " << ps->name() << " ...  ";
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

    Pick::SetIter psiter( *ps );
    while ( psiter.next() )
    {
	const Coord pos = psiter.getPos();
	procstrm << pos.x_ << " " << pos.y_ << "\n";
    }

    strm << "Done" << od_endl;
    return true;
}
