#pragma once

#include "generalmod.h"
#include "enums.h"
#include "od_iosfwd.h"

namespace Gason
{
    enum Tag	{ Number, String, Array, Object, True, False, Null };
    class	Value;
    class	Object;
}


mExpClass(General) IOParTree : public NamedObject
{ mODTextTranslationClass(IOParTree)
public:


    virtual		~IOParTree();

    IOParTree&		operator =(const IOParTree&);

    bool		read(const char*);
    virtual bool	read(od_istream&)				=0;
    virtual bool	write(od_ostream&)				=0;
    bool		isOK() const;
    uiString		errMsg() const		{ return msg_; }

    void		setMessage( const uiString& msg )	{ msg_ = msg; }

protected:

			IOParTree(const char* nm=0);
			IOParTree(const IOParTree&);

    Gason::Value&	data_;
    short		level_;
    short		index_;

    mutable uiString	msg_;

};


namespace Json
{

mExpClass(General) Object : public IOParTree
{ mODTextTranslationClass(Object)
public:

				Object();
				Object(const char* fnm);
				Object(od_istream&);
				Object(const Object&);
				~Object();

	virtual bool		read(od_istream&);
	virtual bool		write(od_ostream&)	{ return false; }
	bool			isGeoJson() const;
	void			setType(Gason::Tag);
	void			setString(const char*);
	void			setNumber(double);
	void			setBool(bool);
	void			set(const Gason::Value&);
	void			get(Gason::Value&) const;
	Gason::Tag		getType() const;
	const char*		getString() const;
	double			getDouble() const;
	bool			getBool() const;

};


mExpClass(General) GeoJsonObject : public NamedObject
{ mODTextTranslationClass(GeoJsonObject)

public:

			GeoJsonObject(const char* fnm);
			GeoJsonObject(od_istream&);
			GeoJsonObject(const GeoJsonObject&);
			~GeoJsonObject();

    bool		fillCollection(const char*);
    bool		fillCollection(od_istream&);
    BufferString	getCollectionName() const;
    BufferString	getCRS() const;
    BufferString	getID() const;
    BufferString	getGeometryType() const;
    Coord3		getCoordinates() const;
    TypeSet<Coord3>	getMultipleCoordinates() const;

    uiString		errMsg() const;

    static const char*	sKeyName()		{ return "name"; }
    static const char*	sKeyType()		{ return "type"; }

private:

    void		setCollectionName();

    Object		jsonobj_;
    BufferString	crs_;
    BufferString	id_;
    BufferString	geomtp_;
    Coord3		crds_;
    TypeSet<Coord3>	multicrds_;
    uiString		msg_;

};

} // namespace GeoJson
