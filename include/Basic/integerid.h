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

    static IntegerID	get( IntType i=-mUdf(IntType) )
					{ return IntegerID(i); }

    IntType		getI() const	{ return id_; }
    void		setI( IntType i ) { id_ = i; }

    inline bool		operator ==( const IntegerID& oth ) const
					{ return id_ == oth.id_;};
    inline bool		operator !=( const IntegerID& oth ) const
					{ return id_ != oth.id_; };
    inline bool		operator >( const IntegerID& oth ) const
					{ return id_ > oth.id_;};
    inline bool		operator >=( const IntegerID& oth ) const
					{ return id_ >= oth.id_;};
    inline bool		operator <( const IntegerID& oth ) const
					{ return id_ < oth.id_;};
    inline bool		operator <=( const IntegerID& oth ) const
					{ return id_ <= oth.id_;};

    inline bool	isBeforeStart() const	{ return mIsUdf(-id_); }
    inline bool	isAfterEnd() const	{ return mIsUdf(id_); };
    inline void	setBeforeStart() const	{ id_ = -mUdf(IntType); }
    inline void	setAfterEnd() const	{ mSetUdf(id_); }

private:

    IntType		id_;

    inline		IntegerID( IntType i=0 )
			    : id_(i)	{ /* keep private */ }

};


#endif
