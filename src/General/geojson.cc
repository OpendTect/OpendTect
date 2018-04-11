#include "geojson.h"

#include "filepath.h"
#include "od_istream.h"
#include "od_ostream.h"


IOParTree::IOParTree( const char* nm )
	: NamedObject( nm )
	, data_( 0 )
	, level_( 0 )
	, index_( 0 )
{
}


IOParTree::IOParTree( const IOParTree& oth )
{ *this = oth; }


IOParTree::~IOParTree()
{
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


namespace Json
{

JsonObject::JsonObject()
	: IOParTree()
{
}


JsonObject::JsonObject( const char* fnm )
	: IOParTree()
{
	IOParTree::read( fnm );
}


JsonObject::JsonObject( od_istream& strm )
	: IOParTree()
{
	read( strm );
}


JsonObject::JsonObject( const JsonObject& obj )
	: IOParTree( obj )
{}


bool JsonObject::read( od_istream& strm )
{
	BufferString datastr;
	datastr = strm.getAll( datastr );
	JsonAllocator alloc;
	char* endptr;
	const int status = jsonParse( datastr.getCStr(), &endptr, &data_, alloc );
	if ( status != JSON_OK )
	{
		msg_ = tr( "Could not parse string" );
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


GeoJsonObject::GeoJsonObject( const char* fnm )
	: NamedObject()
	, crs_( 0 )
	, id_( 0 )
	, geomtp_( 0 )
	, crds_( 0 )
{
	multicrds_.setEmpty();
	fillCollection( fnm );
}


GeoJsonObject::GeoJsonObject( od_istream& strm )
	: NamedObject()
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


uiString GeoJsonObject::errMsg() const
{ return jsonobj_.errMsg(); }


bool GeoJsonObject::fillCollection( const char* fnm )
{
	od_istream strm( fnm );
	if ( !strm.isOK() )
	{
		jsonobj_.setMessage( strm.errMsg() );
		return false;
	}

	return jsonobj_.read( strm );
}


bool GeoJsonObject::fillCollection( od_istream& strm )
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

void GeoJsonObject::setCollectionName()
{
	/*Implement
	setName( collectionnm ); */
}


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
