#ifndef datachar_H
#define datachar_H

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Nov 2000
 Contents:	Binary data interpretation
 RCS:		$Id$
________________________________________________________________________

*/


#include "generalmod.h"
#include "bindatadesc.h"
#include "enums.h"


/*!\brief byte-level data characteristics of stored data.

Used for the interpretation (read or write) of data in buffers that are read
directly from disk into buffer. In that case cross-platform issues arise:
byte-ordering and int/float layout.
The Ibm Format is only supported for the types that are used in SEG-Y sample
data handling. SGI is a future option.

*/

#define mDeclConstr(T,ii,is) \
DataCharacteristics( const T* ) \
: BinDataDesc(ii,is,sizeof(T)), fmt_(Ieee), littleendian_(__islittle__) {} \
DataCharacteristics( const T& ) \
: BinDataDesc(ii,is,sizeof(T)), fmt_(Ieee), littleendian_(__islittle__) {}


mExpClass(General) DataCharacteristics : public BinDataDesc
{
public:

    enum Format		{ Ieee, Ibm };

    Format		fmt_;
    bool		littleendian_;

			DataCharacteristics( bool ii=false, bool is=true,
					     ByteCount n=N4, Format f=Ieee,
					     bool l=__islittle__ )
			: BinDataDesc(ii,is,n)
			, fmt_(f), littleendian_(l)		{}
			DataCharacteristics( const BinDataDesc& bd )
			: BinDataDesc(bd)
			, fmt_(Ieee), littleendian_(__islittle__)	{}

    inline bool		isIeee() const		{ return fmt_ == Ieee; }

			DataCharacteristics( unsigned char c1,unsigned char c2 )
						{ set(c1,c2); }
			DataCharacteristics( const char* s )	{ set(s); }

    virtual int         maxStringifiedSize() const      { return 50; }
    virtual void	toString(char*) const;
    virtual void	set(const char*);
    virtual void	dump(unsigned char&,unsigned char&) const;
    virtual void	set(unsigned char,unsigned char);

			mDeclConstr(signed char,true,true)
			mDeclConstr(short,true,true)
			mDeclConstr(int,true,true)
			mDeclConstr(unsigned char,true,false)
			mDeclConstr(unsigned short,true,false)
			mDeclConstr(unsigned int,true,false)
			mDeclConstr(float,false,true)
			mDeclConstr(double,false,true)
			mDeclConstr(od_int64,true,true)

    bool		operator ==( const DataCharacteristics& dc ) const
			{ return isEqual(dc); }
    bool		operator !=( const DataCharacteristics& dc ) const
			{ return !isEqual(dc); }
    DataCharacteristics& operator =( const BinDataDesc& bd )
			{ BinDataDesc::operator=(bd); return *this; }

    bool		needSwap() const
			{ return (int)nrbytes_ > 1
			      && littleendian_ != __islittle__; }

    enum UserType	{ Auto=0, SI8, UI8, SI16, UI16, SI32, UI32, F32,
			  F64, SI64 };
			DeclareEnumUtils(UserType)
			DataCharacteristics(UserType);
    UserType		userType() const; //!< will return 'nearest'

};

#undef mDeclConstr

#endif

