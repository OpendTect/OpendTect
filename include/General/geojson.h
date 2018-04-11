#pragma once

#include "generalmod.h"

#include "enums.h"
#include "gason.h"


class od_istream;

mExpClass(General) IOParTree : public NamedObject
{ mODTextTranslationClass(IOParTree)
public:

						~IOParTree();
	IOParTree&			operator =(const IOParTree&);

	bool				read(const char*);
	virtual bool		read(od_istream&)				=0;
	virtual bool		write(od_ostream&)				=0;
	bool				isOK() const;
	uiString			errMsg() const		{ return msg_; }

	void				setMessage( const uiString& msg )	{ msg_; }

protected:

						IOParTree(const char* nm=0);
						IOParTree(const IOParTree&);
	JsonValue			data_;
	short				level_;
	short				index_;
	mutable uiString	msg_;
};


namespace Json
{

mExpClass(General) JsonObject : public IOParTree
{ mODTextTranslationClass(JsonObject)
public:

						JsonObject();
						JsonObject(const char* fnm);
						JsonObject(od_istream&);
						JsonObject(const JsonObject&);
						~JsonObject();

	virtual bool		read(od_istream&);
	virtual bool		write(od_ostream&)				{ return false; }
	bool				isGeoJson() const;
	void				setType(JsonTag);
	void				setString(const char*);
	void				setNumber(double);
	void				setBool(bool);
	void				set(JsonValue);
	JsonTag				getType() const;
	const char*			getString() const;
	double				getDouble() const;
	bool				getBool() const;
	JsonValue			getJsonValue() const;

};


mExpClass(General) GeoJsonObject : public NamedObject
{ mODTextTranslationClass(GeoJsonObject)

public:

						GeoJsonObject(const char* fnm);
						GeoJsonObject(od_istream&);
						GeoJsonObject(const GeoJsonObject&);
						~GeoJsonObject();

	bool				fillCollection(const char*);
	bool				fillCollection(od_istream&);
	BufferString		getCollectionName() const;
	BufferString		getCRS() const;
	BufferString		getID() const;
	BufferString		getGeometryType() const;
	Coord3				getCoordinates() const;
	TypeSet<Coord3>		getMultipleCoordinates() const;

	uiString			errMsg() const;

	static const char*	sKeyName()		{ return "name"; }
	static const char*	sKeyType()		{ return "type"; }

private:

	void				setCollectionName();

	JsonObject			jsonobj_;
	BufferString		crs_;
	BufferString		id_;
	BufferString		geomtp_;
	Coord3				crds_;
	TypeSet<Coord3>		multicrds_;
	uiString			msg_;
};

} // namespace GeoJson
