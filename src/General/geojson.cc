#include "geojson.h"

#include "ascstream.h"
#include "filepath.h"
#include "od_istream.h"
#include "od_ostream.h"


IOParTree::IOParTree( const char* nm )
	: NamedObject( nm )
	, data_( 0 )
	, level_( 0 )
	, index_( 0 )
{
	errmsg_.setEmpty();
}


IOParTree::IOParTree( ascistream& strm )
	: NamedObject( 0 )
	, data_( 0 )
	, level_( 0 )
	, index_( 0 )
{
	errmsg_.setEmpty();
}


IOParTree::IOParTree( const IOParTree& oth )
{ *this = oth; }


IOParTree& IOParTree::operator=( const IOParTree& oth )
{
	if ( &oth == this )
		return *this;

	NamedObject::operator=( oth );
	index_ = oth.index_;
	level_ = oth.level_;
	data_ = oth.data_;
	errmsg_ = oth.errmsg_;
	return *this;
}


bool IOParTree::isOK() const
{ return errmsg_.isEmpty(); }


uiString IOParTree::errMsg() const
{ return errmsg_; }


IOParTree::~IOParTree()
{
	data_ = 0;
	index_ = 0;
	level_ = 0;
	errmsg_.setEmpty();
}


namespace Json
{


JsonObject::JsonObject( const char* nm )
	: IOParTree( nm )
{
	read( nm );
}


JsonObject::JsonObject( ascistream& strm )
	: IOParTree( strm )
{
	read( strm );
}


JsonObject::JsonObject( const JsonObject& obj )
	: IOParTree( obj )
{}


bool JsonObject::read( const char* filenm )
{
	od_istream strm( filenm );
	if (!strm.isOK())
	{
		errmsg_ = strm.errMsg();
		return false;
	}
	BufferString datastr;
	strm.getAll( datastr );
	strm.close();
	JsonAllocator alloc;
	char* endptr;
	const int status = jsonParse( datastr.getCStr(), &endptr, &data_, alloc );
	if ( status != JSON_OK )
	{
		errmsg_ = tr( "Could not parse string" );
		return false;
	}
	return true;
}


bool JsonObject::read( ascistream& strm )
{
	BufferString datastr;
	datastr = strm.value();
	JsonAllocator alloc;
	char* endptr;
	const int status = jsonParse( datastr.getCStr(), &endptr, &data_, alloc );
	if ( status != JSON_OK )
	{
		errmsg_ = tr( "Could not parse string" );
		return false;
	}
	return true;
}


bool JsonObject::isGeoJson() const
{ return false; }


void JsonObject::setType( JsonTag tag )
{}


void JsonObject::setString( const char* str )
{}


void JsonObject::setNumber( double nr )
{}


void JsonObject::setBool( bool bl )
{}


void JsonObject::set( JsonValue val )
{}


JsonObject::~JsonObject()
{}


GeoJsonObject::GeoJsonObject( const char* nm )
	: NamedObject( nm )
	, jsonobj_( nm )
	, crs_( 0 )
	, id_( 0 )
	, geomtp_( 0 )
	, crds_( 0 )
{
	multicrds_.setEmpty();
	fillCollection( nm );
}


GeoJsonObject::GeoJsonObject( ascistream& strm )
	: NamedObject( 0 )
	, jsonobj_( strm )
	, crs_( 0 )
	, id_( 0 )
	, geomtp_( 0 )
	, crds_( 0 )
{
	multicrds_.setEmpty();
	fillCollection( strm );
}


GeoJsonObject::GeoJsonObject( const GeoJsonObject& oth )
	: NamedObject( name() )
	, jsonobj_( oth.jsonobj_ )
{ *this = oth; }


bool GeoJsonObject::fillCollection( const char* str )
{ return jsonobj_.read( str ); }


bool GeoJsonObject::fillCollection( ascistream& strm )
{ return jsonobj_.read( strm ); }


BufferString GeoJsonObject::getCollectionName() const
{ return BufferString( name() ); }


BufferString GeoJsonObject::getCRS() const
{ return crs_; }


BufferString GeoJsonObject::getID() const
{ return id_; }


BufferString GeoJsonObject::getGeometryType() const
{ return geomtp_; }


Coord3 GeoJsonObject::getCoordinates() const
{ return crds_; }


TypeSet<Coord3> GeoJsonObject::getMultipleCoordinates() const
{ return multicrds_; }


GeoJsonObject::~GeoJsonObject()
{
	jsonobj_ = 0;
	crs_ = 0;
	id_ = 0;
	geomtp_ = 0;
	crds_ = 0;
	multicrds_.setEmpty();
}


}; //namespace Json
