#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "undefval.h"
#include "bufstring.h"


/*!\brief Single integer ID with comparison but no automatic conversion.
  Making a subclass is optional, but recommended when using multiple
  IntegerID objects at the same time.
  Subclassing is mandatory when you want a customized undefined value or
  a different condition for valid values.

  Example:

  using ID = IntegerID<od_uint16>;

  void dummy( ID id )
  {
      id.set( 666 ); // good
      id = 666; // error
      id += 666; // error
      id.set( id.asInt() + 666 ); // good
  }

  void dummy2()
  {
      dummy( ID::get(555) ); // good
      dummy( 555 ); // error
  }

  class X { X(); ID id_; };

  X::X() : id_(ID::get(444)) {} // good
  X::X() : id_(444) {} // not recommended, but will compile.

*/


template <class IntType=od_uint16>
mClass(Basic) IntegerID
{
public:
    inline			IntegerID()
				{ nr_ = udfVal(); }
    inline explicit		IntegerID( IntType nr )
				    : nr_(nr)		{}
    virtual			~IntegerID()		{}

    static inline IntegerID	get( IntType nr )
				{ return IntegerID( nr ); }

    inline IntType		asInt() const		{ return nr_; }
    inline void			set( IntType i )	{ nr_ = i; }
    inline void			fromString(const char* s,
					   IntType defval=mUdf(IntType))
				{ nr_ = toInt(s,defval); }
    inline BufferString		toString() const
				{ return ::toString(asInt()); }

    inline virtual bool		operator==( const IntegerID& oth ) const
				{ return this->nr_ == oth.nr_; }
    inline virtual bool		operator!=( const IntegerID& oth ) const
				{ return this->nr_ != oth.nr_; }

    inline virtual bool		isValid() const { return nr_>=0 && !isUdf(); }
    inline bool			isUdf() const	{ return nr_ == udfVal(); }
    inline void			setUdf()	{ nr_ = udfVal(); }
    static inline IntegerID	udf()		{ return IntegerID(); }

protected:
    virtual IntType		udfVal() const	{ return mUdf(IntType); }

private:
    IntType			nr_;
};

// For convenience
using Int8ID = IntegerID<od_int8>;
using Int16ID = IntegerID<od_int16>;
using Int32ID = IntegerID<od_int32>;
using Int64ID = IntegerID<od_int64>;
using UInt8ID = IntegerID<od_uint8>;
using UInt16ID = IntegerID<od_uint16>;
using UInt32ID = IntegerID<od_uint32>;
using UInt64ID = IntegerID<od_uint64>;

// Specific usage
using RandomLineID	= UInt16ID;

mExpClass(Basic) SynthID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;

    bool			isValid() const override
				{ return asInt()>0 && !isUdf(); }
    bool			isNone() const		{ return asInt()==0; }
    static inline SynthID	udf()			{ return SynthID(); }
};


mExpClass(Basic) VisID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline VisID		udf()			{ return VisID(); }
};

using SceneID = VisID;


mExpClass(Basic) Viewer2DID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline Viewer2DID	udf()			{ return Viewer2DID(); }
};


mExpClass(Basic) Vis2DID : public IntegerID<od_int32>
{
public:
    using IntegerID::IntegerID;
    static inline Vis2DID	udf()			{ return Vis2DID(); }
};
