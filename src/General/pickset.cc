/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2001
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "pickset.h"

#include "ioman.h"
#include "iopar.h"
#include "multiid.h"
#include "polygon.h"
#include "separstr.h"
#include "survinfo.h"
#include "tabledef.h"
#include "posimpexppars.h"
#include "unitofmeasure.h"
#include <ctype.h>


static char pipechar = '|';
static char newlinechar = '\n';


namespace Pick
{

Location::Location( double x, double y, double z )
    : pos_(x,y,z), text_(0)
{}

Location::Location( const Coord& c, float z )
    : pos_(c,z), text_(0)
{}

Location::Location( const Coord3& c )
    : pos_(c), text_(0)
{}

Location::Location( const Coord3& c, const Coord3& d )
    : pos_(c), dir_(d), text_(0)
{}

Location::Location( const Coord3& c, const Sphere& d )
    : pos_(c), dir_(d), text_(0)
{}

Location::Location( const Location& pl )
    : text_(0)
{ *this = pl; }


Location::~Location()
{
    if ( text_ ) delete text_;
}


void Location::operator=( const Location& pl )
{
    pos_ = pl.pos_;
    dir_ = pl.dir_;
    if ( pl.text_ )
    {
	if ( !text_ )
	    text_ = new BufferString( *pl.text_ );
	else
	    *text_ = *pl.text_;
    }
}


void Location::setText( const char* key, const char* txt )
{
    unSetText( key );
    if ( !text_ ) text_ = new BufferString;
    SeparString sepstr( *text_, '\'' );

    sepstr.add( key );
    sepstr.add( txt );
    *text_ = sepstr;
}


void Location::unSetText( const char* key )
{
    if ( !text_ ) return;
    SeparString sepstr( *text_, '\'' );
    for ( int idx=0; idx<sepstr.size(); idx+=2 )
    {
	if ( key!=sepstr[idx] )
	    continue;

	SeparString copy( 0, '\'' );
	const int nrkeys = sepstr.size();
	for ( int idy=0; idy<nrkeys; idy++ )
	{
	    if ( idy==idx || idy==idx+1 )
		continue;

	    copy.add( sepstr[idy] );
	}

	sepstr = copy;
	idx-=2;
    }

    (*text_) = sepstr;
}


static double getNextVal( char*& str )
{
    if ( !*str ) return mUdf(double);
    char* endptr = str; mSkipNonBlanks( endptr );
    if ( *endptr ) *endptr++ = '\0';
    double v = toDouble( str );
    str = endptr; mSkipBlanks(str);
    return v;
}


bool Location::fromString( const char* s, bool doxy, bool testdir )
{
    if ( !s || !*s ) return false;

    if ( *s == '"' )
    {
	s++;

	if ( !text_ ) text_ = new BufferString( s );
	else *text_ = s;

	char* start = text_->buf();
	char* stop = strchr( start, '"' );
	if ( !stop )
	{
	    delete text_;
	    text_ = 0;
	}
	else
	{
	    *stop = '\0';
	    s += stop - start + 1;
	    replaceCharacter( text_->buf(), newlinechar, pipechar );
	}
    }
    else if ( text_ )
    {
	delete text_;
	text_ = 0;
    }

    BufferString bufstr( s );
    char* str = bufstr.buf();
    mSkipBlanks(str);

    double xread = getNextVal( str );
    double yread = getNextVal( str );
    double zread = getNextVal( str );
    if ( mIsUdf(zread) )
	return false;

    pos_.x = xread;
    pos_.y = yread;
    pos_.z = zread;

    // Check if data is in inl/crl rather than X and Y
    if ( !SI().isReasonable(pos_) || !doxy )
    {
	BinID bid( mNINT32(pos_.x), mNINT32(pos_.y) );
	SI().snap( bid, BinID(0,0) );
	Coord newpos_ = SI().transform( bid );
	if ( SI().isReasonable(newpos_) )
	{
	    pos_.x = newpos_.x;
	    pos_.y = newpos_.y;
	}
    }

    // See if there's a direction, too
    xread = getNextVal( str );
    yread = getNextVal( str );
    zread = getNextVal( str );

    if ( testdir )
    {
	if ( !mIsUdf(yread) )
	{
	    if ( mIsUdf(zread) ) zread = 0;
	    dir_ = Sphere( ( float ) xread,( float )  yread,( float )  zread );
	}
    }
    else
	dir_ = Sphere( ( float )  xread,( float )  yread,( float )  zread );

    return true;
}


void Location::toString( BufferString& str, bool forexport ) const
{
    str = "";
    if ( text_ && *text_ )
    {
	BufferString txt( *text_ );
	replaceCharacter( txt.buf(), newlinechar, pipechar );
	str = "\""; str += txt; str += "\"\t";
    }

    Coord3 usepos( pos_ );
    if ( forexport )
    {
	mPIEPAdj(Coord,usepos,false);
	if ( mPIEP.haveZChg() )
	{
	    float z = (float)usepos.z;
	    mPIEPAdj(Z,z,false);
	    usepos.z = z;
	}
    }

#define mSetFormat( val ) \
    fmt = val<1e-1 ? "%le" : ((val>=1e-1 && val<1e8) ? "%lf" : "%lg")

    str.setMinBufSize( str.size()+1024 );
    const char* fmt = 0;
   
    mSetFormat(usepos.x);
    getStringFromDouble( fmt, usepos.x, str.bufEnd() );
    mSetFormat(usepos.y);
    str += "\t"; getStringFromDouble( fmt, usepos.y, str.bufEnd() );
    mSetFormat(usepos.z); 
    str += "\t"; getStringFromDouble( fmt, usepos.z, str.bufEnd() );

    if ( hasDir() )
    {
	str += "\t"; getStringFromDouble( 0, dir_.radius, str.bufEnd() );
	str += "\t"; getStringFromDouble( 0, dir_.theta, str.bufEnd() );
	str += "\t"; getStringFromDouble( 0, dir_.phi, str.bufEnd() );
    }
}


bool Location::getText( const char* idkey, BufferString& val ) const
{
    if ( !text_ )
    {
	val = "";
	return false;
    }
    
    SeparString sepstr( *text_, '\'' );
    const int strsz = sepstr.size();
    if ( !strsz ) return false;

    for ( int idx=0; idx<strsz; idx+=2 )
    {
	if ( idkey!=sepstr[idx] )
	    continue;

	val = sepstr[idx+1];
	return true;
    }

    return false;
}


// Pick::SetMgr
SetMgr& SetMgr::getMgr( const char* nm )
{
    static ObjectSet<SetMgr>* mgrs = 0;
    SetMgr* newmgr = 0;
    if ( !mgrs )
    {
	mgrs = new ObjectSet<SetMgr>;
	newmgr = new SetMgr( 0 );
	    // ensure the first mgr has the 'empty' name
    }
    else if ( (!nm || !*nm) )
	return *((*mgrs)[0]);
    else
    {
	for ( int idx=1; idx<mgrs->size(); idx++ )
	{
	    if ( (*mgrs)[idx]->name() == nm )
		return *((*mgrs)[idx]);
	}
    }

    if ( !newmgr )
	newmgr = new SetMgr( nm );
    *mgrs += newmgr;
    IOM().surveyToBeChanged.notify( mCB(newmgr,SetMgr,survChg) );
    IOM().entryRemoved.notify( mCB(newmgr,SetMgr,objRm) );
    return *newmgr;
}


SetMgr::SetMgr( const char* nm )
    : NamedObject(nm)
    , locationChanged(this), setToBeRemoved(this)
    , setAdded(this), setChanged(this)
    , setDispChanged(this)
{}


void SetMgr::add( const MultiID& ky, Set* st )
{
    pss_ += st; ids_ += ky; changed_ += false;
    setAdded.trigger( st );
}


void SetMgr::set( const MultiID& ky, Set* newset )
{
    Set* oldset = find( ky );
    if ( !oldset )
    {
	if ( newset )
	    add( ky, newset );
    }
    else if ( newset != oldset )
    {
	const int idx = pss_.indexOf( oldset );
	//Must be removed from list before trigger, otherwise
	//other users may remove it in calls invoked by the cb.
	pss_.removeSingle( idx ); 
	setToBeRemoved.trigger( oldset );
	delete oldset; 
	ids_.removeSingle( idx );
	changed_.removeSingle( idx );
	if ( newset )
	    add( ky, newset );
    }
}


const MultiID& SetMgr::id( int idx ) const
{ return ids_[idx]; }


void SetMgr::setID( int idx, const MultiID& mid )
{
    ids_[idx] = mid;
}


int SetMgr::indexOf( const Set& st ) const
{
    return pss_.indexOf( &st );
}


int SetMgr::indexOf( const MultiID& ky ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( ky == ids_[idx] )
	    return idx;
    }
    return -1;
}


int SetMgr::indexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	if ( pss_[idx]->name() == nm )
	    return idx;
    }
    return -1;
}


Set* SetMgr::find( const MultiID& ky ) const
{
    const int idx = indexOf( ky );
    return idx < 0 ? 0 : const_cast<Set*>( pss_[idx] );
}


MultiID* SetMgr::find( const Set& st ) const
{
    const int idx = indexOf( st );
    return idx < 0 ? 0 : const_cast<MultiID*>( &ids_[idx] );
}


Set* SetMgr::find( const char* nm ) const
{
    const int idx = indexOf( nm );
    return idx < 0 ? 0 : const_cast<Set*>( pss_[idx] );
}


void SetMgr::reportChange( CallBacker* sender, const ChangeData& cd )
{
    const int setidx = pss_.indexOf( cd.set_ );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	locationChanged.trigger( const_cast<ChangeData*>( &cd ), sender );
    }
}


void SetMgr::reportChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	setChanged.trigger( const_cast<Set*>(&s), sender );
    }
}


void SetMgr::reportDispChange( CallBacker* sender, const Set& s )
{
    const int setidx = pss_.indexOf( &s );
    if ( setidx >= 0 )
    {
	changed_[setidx] = true;
	setDispChanged.trigger( const_cast<Set*>(&s), sender );
    }
}


void SetMgr::removeCBs( CallBacker* cb )
{
    locationChanged.removeWith( cb );
    setToBeRemoved.removeWith( cb );
    setAdded.removeWith( cb );
    setChanged.removeWith( cb );
    setDispChanged.removeWith( cb );
}


void SetMgr::survChg( CallBacker* )
{
    locationChanged.cbs_.erase();
    setToBeRemoved.cbs_.erase();
    setAdded.cbs_.erase();
    setChanged.cbs_.erase();
    setDispChanged.cbs_.erase();
    deepErase( pss_ );
    ids_.erase();
    changed_.erase();
}


void SetMgr::objRm( CallBacker* cb )
{
    mCBCapsuleUnpack(MultiID,ky,cb);
    if ( indexOf(ky) >= 0 )
	set( ky, 0 );
}


// Pick::Set
DefineEnumNames( Set::Disp, Connection, 0, "Connection" )
{ "None", "Open", "Close", 0 };

Set::Set( const char* nm )
    : NamedObject(nm)
    , pars_(*new IOPar)
{}


Set::Set( const Set& s )
    : pars_(*new IOPar)
{ *this = s; }

Set::~Set()
{ delete &pars_; }


Set& Set::operator=( const Set& s )
{
    if ( &s == this ) return *this;
    copy( s ); setName( s.name() );
    disp_ = s.disp_; pars_ = s.pars_;
    return *this;
}


float Set::getXYArea() const
{
    if ( size()<3 || disp_.connect_==Set::Disp::None )
	return mUdf(float);
    
    TypeSet<Geom::Point2D<float> > posxy;
    for ( int idx=size()-1; idx>=0; idx-- )
    {
	const Coord localpos = (*this)[idx].pos_;
	posxy += Geom::Point2D<float>(( float )localpos.x,( float )localpos.y);
    }

    ODPolygon<float> polygon( posxy );
    if ( polygon.isSelfIntersecting() )
	return mUdf(float);

    float area = polygon.area();
    if ( SI().xyInFeet() )
	area *= (mFromFeetFactorF*mFromFeetFactorF);

    return area;
}


static const char* sKeyConnect = "Connect";

void Set::fillPar( IOPar& par ) const
{
    BufferString colstr;
    if ( disp_.color_ != Color::NoColor() )
    {
	disp_.color_.fill( colstr.buf() );
	par.set( sKey::Color(), colstr.buf() );
    }

    par.set( sKey::Size(), disp_.pixsize_ );
    par.set( sKeyMarkerType(), disp_.markertype_ );
    par.set( sKeyConnect, Disp::getConnectionString(disp_.connect_) );
    par.merge( pars_ );
}


bool Set::usePar( const IOPar& par )
{
    BufferString colstr;
    if ( par.get(sKey::Color(),colstr) )
	disp_.color_.use( colstr.buf() );

    disp_.pixsize_ = 3;
    par.get( sKey::Size(), disp_.pixsize_ );
    par.get( sKeyMarkerType(), disp_.markertype_ );

    bool doconnect;
    par.getYN( sKeyConnect, doconnect );	// For Backward Compatibility
    if ( doconnect ) disp_.connect_ = Disp::Close;
    else
    {
	if ( !Disp::parseEnumConnection(par.find(sKeyConnect), disp_.connect_) )
	    disp_.connect_ = Disp::None;
    }

    pars_ = par;
    pars_.removeWithKey( sKey::Color() );
    pars_.removeWithKey( sKey::Size() );
    pars_.removeWithKey( sKeyMarkerType() );
    pars_.removeWithKey( sKeyConnect );
    return true;
}


} // namespace Pick


// PickSetAscIO
Table::FormatDesc* PickSetAscIO::getDesc( bool iszreq )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "PickSet" );
    createDescBody( fd, iszreq );
    return fd;
}


void PickSetAscIO::createDescBody( Table::FormatDesc* fd, bool iszreq )
{
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    if ( iszreq )
	fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
}


void PickSetAscIO::updateDesc( Table::FormatDesc& fd, bool iszreq )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, iszreq );
}
    

bool PickSetAscIO::isXY() const
{
    return formOf( false, 0 ) == 0;
}


#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

bool PickSetAscIO::get( std::istream& strm, Pick::Set& ps,
			bool iszreq, float constz ) const
{
    while ( true )
    {
	int ret = getNextBodyVals( strm );
	if ( ret < 0 ) mErrRet(errmsg_)
	if ( ret == 0 ) break;

	const double xread = getdValue( 0 );
	const double yread = getdValue( 1 );
	if ( mIsUdf(xread) || mIsUdf(yread) ) continue;

	Coord pos( xread, yread );
	mPIEPAdj(Coord,pos,true);
	if ( !isXY() || !SI().isReasonable(pos) )
	{
	    BinID bid( mNINT32(xread), mNINT32(yread) );
	    mPIEPAdj(BinID,bid,true);
	    SI().snap( bid );
	    pos = SI().transform( bid );
	}

	if ( !SI().isReasonable(pos) ) continue;

	float zread = constz;
	if ( iszreq )
	{
	    zread = getfValue( 2 );
	    if ( mIsUdf(zread) )
		continue;

	    mPIEPAdj(Z,zread,true);
	}

	Pick::Location ploc( pos, zread );
	ps += ploc;
    }

    return true;
}

