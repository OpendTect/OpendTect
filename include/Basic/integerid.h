#ifndef opaqueid_h
#define opaqueid_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2016
________________________________________________________________________

-*/

#include "undefval.h"
template <class T> class TypeSet;


/*!\brief single integer ID with comparison but no automatic conversion.
  Note that you will want to make a subclass to make the IDs specific for your
  class. Therefore, use the mDefIntegerID macro.

  typedef IntegerID<int> ID;

  void dummy( ID id )
  {
      id.setI( 666 ); // fine
      id = 666; // nope
      id += 666; // nope (good thing)
      id.setI( id.getI() + 666 ); // fine (weird, but you wanted it)
  }

  void dummy2()
  {
      dummy( ID::get(555) ); // fine
      dummy( 555 ); // nope
  }

  class X { X(); ID id_; };

  X::X() : id_(ID::get(444)) {} // fine
  X::X() : id_(444) {} // nope
  X::X() {} // nope

*/


template <class IntType>
mClass(Basic) IntegerID
{
public:

    typedef IntType	IDType;

    static inline IntegerID get( IntType i )
					{ return IntegerID(i); }

    inline IntType	getI() const	{ return id_; }
    inline void		setI( IntType i ) { id_ = i; }

    inline bool		operator ==( const IntegerID& oth ) const
					{ return id_ == oth.id_;};
    inline bool		operator !=( const IntegerID& oth ) const
					{ return id_ != oth.id_;};
				// Do not add '>' or similar!

    inline bool		isInvalid() const	{ return id_ < 0; }
    inline bool		isValid() const		{ return !isInvalid(); }
    inline void		setInvalid()		{ id_ = -1; }
    static inline IntegerID getInvalid()	{ return IntegerID(-1); }

protected:

    IntType		id_;

    inline		IntegerID( IntType i=0 )
			    : id_(i)	{ /* keep private! */ }

    friend class	TypeSet<IntType>;

};


#define mDefIntegerIDType(IntType,classname) \
 \
class classname : public IntegerID<IntType> \
{ \
public: \
 \
    static inline classname get( IntType i ) \
					{ return classname(i); } \
 \
    inline bool		operator ==( const classname& oth ) const \
			{ return IntegerID<IntType>::operator ==(oth); } \
    inline bool		operator !=( const classname& oth ) const \
			{ return IntegerID<IntType>::operator !=(oth); } \
 \
    static inline classname getInvalid() { return classname(-1); } \
 \
protected: \
 \
    inline		classname( IntType i=0 ) \
			    : IntegerID<IntType>(i)	{} \
 \
    friend class	TypeSet<classname>; \
 \
}


#endif
