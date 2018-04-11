#pragma once

#include "generalmod.h"

#include "enums.h"
#include "gason.h"


class ascistream;
class ascostream;

mExpClass(General) IOParTree : public NamedObject
{ mODTextTranslationClass(IOParTree)
public:

						~IOParTree();
	IOParTree&			operator =(const IOParTree&);
	virtual bool		read(const char*)				=0;
	virtual bool		read(ascistream&)				=0;
	virtual bool		write(od_ostream&)				=0;
	bool				isOK() const;
	uiString			errMsg() const;

protected:

						IOParTree(const char*);
						IOParTree(ascistream&);
						IOParTree(const IOParTree&);
	JsonValue			data_;
	short				level_;
	short				index_;
	mutable uiString	errmsg_;
};


namespace Json
{

mExpClass(General) JsonObject : public IOParTree
{ mODTextTranslationClass(JsonObject)
public:

						JsonObject(const char*);
						JsonObject(ascistream&);
						JsonObject(const JsonObject&);
						~JsonObject();
	virtual bool		read(const char*);
	virtual bool		read(ascistream&);
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

						GeoJsonObject(const char*);
						GeoJsonObject(ascistream&);
						GeoJsonObject(const GeoJsonObject&);
						~GeoJsonObject();
	bool				fillCollection(const char*);
	bool				fillCollection(ascistream&);
	BufferString		getCollectionName() const;
	BufferString		getCRS() const;
	BufferString		getID() const;
	BufferString		getGeometryType() const;
	Coord3				getCoordinates() const;
	TypeSet<Coord3>		getMultipleCoordinates() const;

private:

	JsonObject			jsonobj_;
	BufferString		crs_;
	BufferString		id_;
	BufferString		geomtp_;
	Coord3				crds_;
	TypeSet<Coord3>		multicrds_;
};

} // namespace GeoJson
