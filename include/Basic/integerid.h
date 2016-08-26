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

    inline bool	isInvalid() const	{ return mIsUdf(id_); }
    inline bool	isValid() const		{ return !isInvalid(); }
    inline void	setInvalid()		{ mSetUdf(id_); }
    static inline IntegerID getInvalid() { return IntegerID(mUdf(IntType)); }

private:

    IntType		id_;

    inline		IntegerID( IntType i=0 )
			    : id_(i)	{ /* keep private! */ }

    friend class	TypeSet<IntType>;

};


/*!\brief two integer IDs lumped together. */

template <class IntType>
mClass(Basic) DualID
{
public:

    typedef IntType		    IDType;
    typedef IntegerID<IntType>	    IntIDType;

    static inline DualID get( IntType i1, IntType i2 )
					{ return DualID(i1,i2); }

    inline IntIDType	get1() const	{ return IntIDType::get(id1_); }
    inline IntIDType	get2() const	{ return IntIDType::get(id2_); }
    inline IntType	getI1() const	{ return id1_; }
    inline IntType	getI2() const	{ return id2_; }
    inline void		set1( IntIDType id ) { id1_ = id.getI(); }
    inline void		set2( IntIDType id ) { id2_ = id.getI(); }
    inline void		setI1( IntType i ) { id1_ = i; }
    inline void		setI2( IntType i ) { id2_ = i; }

    inline bool		operator ==( const DualID& oth ) const
				{ return id1_ == oth.id1_ && id2_ == oth.id2_;};
    inline bool		operator !=( const DualID& oth ) const
				{ return id1_ != oth.id1_ || id2_ != oth.id2_;};

    inline bool	isInvalid() const	{ return mIsUdf(id1_) || mIsUdf(id2_); }
    inline bool	isValid() const		{ return !isInvalid(); }
    inline void	setInvalid()		{ setInvalid1(); setInvalid2(); }
    inline void	setInvalid1()		{ mSetUdf(id1_); }
    inline void	setInvalid2()		{ mSetUdf(id2_); }
    static inline DualID getInvalid()
				{ return DualID(mUdf(IntType),mUdf(IntType)); }

private:

    IntType		id1_;
    IntType		id2_;

    inline		DualID( IntType i1=0, IntType i2=0 )
			    : id1_(i1), id2_(i2) { /* keep private! */ }

};


#endif
