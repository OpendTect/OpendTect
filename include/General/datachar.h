#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bindatadesc.h"
#include "enums.h"

#define mDeclConstr(T,ii,is) \
DataCharacteristics( const T* ) \
: BinDataDesc(ii,is,sizeof(T)), fmt_(Ieee), littleendian_(__islittle__) {} \
DataCharacteristics( const T& ) \
: BinDataDesc(ii,is,sizeof(T)), fmt_(Ieee), littleendian_(__islittle__) {}

/*!
\brief byte-level data characteristics of stored data.

  Used for the interpretation (read or write) of data in buffers that are read
  directly from disk into buffer. In that case cross-platform issues arise:
  byte-ordering and int/float layout.
  The IBM Format is only supported for the types that are used in SEG-Y sample
  data handling. SGI is a future option.
*/

mExpClass(General) DataCharacteristics : public BinDataDesc
{
public:

    enum Format		{ Ieee, Ibm };
    enum UserType	{ Auto=0, SI8, UI8, SI16, UI16, SI32, UI32, F32,
			  F64, SI64, UI64 };
			mDeclareEnumUtils(UserType)


    Format		fmt_;
    bool		littleendian_;

			DataCharacteristics(bool ii=false,bool is=true,
					    ByteCount n=N4, Format f=Ieee,
					    bool l=__islittle__);
			DataCharacteristics(OD::DataRepType);
			DataCharacteristics(UserType);
			DataCharacteristics(const BinDataDesc&);
			~DataCharacteristics();

    inline bool		isIeee() const		{ return fmt_ == Ieee; }

			DataCharacteristics( unsigned char c1,unsigned char c2 )
						{ set(c1,c2); }
			DataCharacteristics( const char* s )	{ set(s); }

    int			maxStringifiedSize() const override	 { return 50; }
    void		toString(BufferString&) const override;
    void		set(const char*) override;
    void		dump(unsigned char&,unsigned char&) const override;
    void		set(unsigned char,unsigned char) override;

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

    UserType		userType() const; //!< will return 'nearest'
    static bool		getUserTypeFromPar(const IOPar&,UserType&);
    static void		putUserTypeToPar(IOPar&,UserType);
    void		putUserType( IOPar& iop ) const
			{ putUserTypeToPar( iop, userType() ); }

    double		getLimitValue(bool max) const;

};

#undef mDeclConstr
