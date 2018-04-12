/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Arnaud
 * DATE     : April 2018
-*/

#include "geojson.h"
#include "gason.h"

#include "filepath.h"
#include "od_iostream.h"


static inline JsonTag getJsonTag( Gason::Tag t ) { return (JsonTag)t; }
static inline Gason::Tag getGasonTag( JsonTag t ) { return (Gason::Tag)t; }

namespace Gason
{
    struct Value
    {
	Value( double x )	: jval_( x )		{}
	Value( JsonTag tag=JSON_NULL, void* payload = nullptr )
				: jval_( tag, payload )	{}
	JsonValue   jval_;

	operator JsonValue&()			{ return jval_; }
	operator const JsonValue&() const	{ return jval_; }
	Value& operator =( const Value& oth )
		{ jval_ = oth.jval_; return *this; }
    };
}


IOParTree::IOParTree( const char* nm )
    : NamedObject( nm )
    , data_( *new Gason::Value )
    , level_( 0 )
    , index_( 0 )
{
}


IOParTree::IOParTree( const IOParTree& oth )
    : data_( *new Gason::Value )
{
    *this = oth;
}


IOParTree::~IOParTree()
{
    delete &data_;
}


IOParTree& IOParTree::operator=( const IOParTree& oth )
{
    if ( &oth == this )
	return *this;

    NamedObject::operator=( oth );
    index_ = oth.index_;
    level_ = oth.level_;
    data_ = oth.data_;
    msg_ = oth.msg_;
    return *this;
}


bool IOParTree::isOK() const
{ return msg_.isEmpty(); }



bool IOParTree::read( const char* filenm )
{
    od_istream strm( filenm );
    if (!strm.isOK())
    {
	msg_ = strm.errMsg();
	return false;
    }

    return read( strm );
}


Json::Object::Object()
    : IOParTree()
{
}


Json::Object::Object( const char* fnm )
    : IOParTree()
{
    IOParTree::read( fnm );
}


Json::Object::Object( od_istream& strm )
    : IOParTree()
{
	read( strm );
}


Json::Object::Object( const Object& obj )
    : IOParTree( obj )
{}


bool Json::Object::read( od_istream& strm )
{
    BufferString datastr;
    datastr = strm.getAll( datastr );
    JsonAllocator alloc;
    char* endptr;
    const int status = jsonParse( datastr.getCStr(), &endptr, &data_.jval_,
				  alloc );
    if ( status != JSON_OK )
	{ msg_ = tr( "Could not parse string" ); return false; }
    return true;
}


bool Json::Object::isGeoJson() const
{
    return false;
}


void Json::Object::setType( Gason::Tag tag )
{
}


void Json::Object::setString( const char* str )
{}


void Json::Object::setNumber( double nr )
{}


void Json::Object::setBool( bool bl )
{}


void Json::Object::set( const Gason::Value& val )
{
}


void Json::Object::get( Gason::Value& val ) const
{
}


Json::Object::~Object()
{
}


Json::GeoJsonObject::GeoJsonObject( const char* fnm )
    : NamedObject()
    , crs_( 0 )
    , id_( 0 )
    , geomtp_( 0 )
    , crds_( 0 )
{
    multicrds_.setEmpty();
    fillCollection( fnm );
}


Json::GeoJsonObject::GeoJsonObject( od_istream& strm )
    : NamedObject()
    , crs_( 0 )
    , id_( 0 )
    , geomtp_( 0 )
    , crds_( 0 )
{
    multicrds_.setEmpty();
    fillCollection( strm );
}


Json::GeoJsonObject::GeoJsonObject( const GeoJsonObject& oth )
	: NamedObject( name() )
	, jsonobj_( oth.jsonobj_ )
{ *this = oth; }


uiString Json::GeoJsonObject::errMsg() const
{ return jsonobj_.errMsg(); }


bool Json::GeoJsonObject::fillCollection( const char* fnm )
{
    od_istream strm( fnm );
    if ( !strm.isOK() )
    {
	jsonobj_.setMessage( strm.errMsg() );
	return false;
    }

    return jsonobj_.read( strm );
}


bool Json::GeoJsonObject::fillCollection( od_istream& strm )
{
    const bool parsesuccess = jsonobj_.read( strm );
    if ( parsesuccess )
    {
	// check type is good
	setCollectionName();
	// set CRS
    }

    return parsesuccess;
}

void Json::GeoJsonObject::setCollectionName()
{
    /*Implement
    setName( collectionnm ); */
}


BufferString Json::GeoJsonObject::getCollectionName() const
{ return BufferString( name() ); }


BufferString Json::GeoJsonObject::getCRS() const
{ return crs_; }


BufferString Json::GeoJsonObject::getID() const
{ return id_; }


BufferString Json::GeoJsonObject::getGeometryType() const
{ return geomtp_; }


Coord3 Json::GeoJsonObject::getCoordinates() const
{ return crds_; }


TypeSet<Coord3> Json::GeoJsonObject::getMultipleCoordinates() const
{ return multicrds_; }


Json::GeoJsonObject::~GeoJsonObject()
{
    jsonobj_ = 0;
    crs_ = 0;
    id_ = 0;
    geomtp_ = 0;
    crds_ = 0;
    multicrds_.setEmpty();
}
