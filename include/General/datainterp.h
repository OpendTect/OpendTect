#ifndef datainterp_H
#define datainterp_H

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Nov 2000
 Contents:	Binary data interpretation
 RCS:		$Id: datainterp.h,v 1.6 2003-11-07 12:21:51 bert Exp $
________________________________________________________________________

*/


#include <general.h>
class DataCharacteristics;


/*!\brief Byte-level data interpreter.

Efficient (one function call overhead) get and set of data, usually in a data
buffer. Facility to swap bytes in advance. The interpretation is into/from
the template parameter. At present, only float, double and int are supported
and instantiated.

*/

template<class T>
class DataInterpreter
{
public:

			DataInterpreter(const DataCharacteristics&,
					bool ignoreendianness=false);
			DataInterpreter(const DataInterpreter<T>&);
    void		set(const DataCharacteristics&,
			    bool ignoreendianness=false);
			//!< use ignoreendianness when you pre-byteswap the data
    DataInterpreter<T>&	operator=(const DataInterpreter<T>&);
    inline DataInterpreter<T>& operator=( const DataCharacteristics& dc )
			{ set( dc, false ); return *this; }

    bool		needSwap() const;
    void		swap( void* buf, int bufsz_in_elements ) const
			{ (this->*swpfn)( buf, bufsz_in_elements );
			  const_cast<DataInterpreter<T>*>(this)->swpSwap(); }

    inline T		get( const void* buf, int nr ) const
			{ return (this->*getfn)( buf, nr ); }
    inline void		put( void* buf, int nr, T t ) const
			{ (this->*putfn)( buf, nr, t ); }

    inline bool		operator ==( const DataInterpreter& di ) const
			{ return di.getfn == getfn; }
    inline bool		operator !=( const DataInterpreter& di ) const
			{ return di.getfn != getfn; }
    inline bool		isSUCompat() const
			{ return getfn == &DataInterpreter::getF; }
    int			nrBytes() const;
    DataCharacteristics	dataChar() const;

protected:

    void		swap2(void*,int) const;
    void		swap4(void*,int) const;
    void		swap8(void*,int) const;

    T			getS1(const void*,int) const;
    T			getS2(const void*,int) const;
    T			getS4(const void*,int) const;
    T			getS8(const void*,int) const;
    T			getU1(const void*,int) const;
    T			getU2(const void*,int) const;
    T			getU4(const void*,int) const;
    T			getU8(const void*,int) const;
    T			getF(const void*,int) const;
    T			getD(const void*,int) const;

    T			getS2Ibm(const void*,int) const;
    T			getS4Ibm(const void*,int) const;
    T			getFIbm(const void*,int) const;

    T			getS2swp(const void*,int) const;
    T			getS4swp(const void*,int) const;
    T			getS8swp(const void*,int) const;
    T			getU2swp(const void*,int) const;
    T			getU4swp(const void*,int) const;
    T			getU8swp(const void*,int) const;
    T			getFswp(const void*,int) const;
    T			getDswp(const void*,int) const;

    T			getS2Ibmswp(const void*,int) const;
    T			getS4Ibmswp(const void*,int) const;
    T			getFIbmswp(const void*,int) const;

    void		putS1(void*,int,T) const;
    void		putS2(void*,int,T) const;
    void		putS4(void*,int,T) const;
    void		putS8(void*,int,T) const;
    void		putU1(void*,int,T) const;
    void		putU2(void*,int,T) const;
    void		putU4(void*,int,T) const;
    void		putU8(void*,int,T) const;
    void		putF(void*,int,T) const;
    void		putD(void*,int,T) const;

    void		putS2Ibm(void*,int,T) const;
    void		putS4Ibm(void*,int,T) const;
    void		putFIbm(void*,int,T) const;

    void		putS2swp(void*,int,T) const;
    void		putS4swp(void*,int,T) const;
    void		putS8swp(void*,int,T) const;
    void		putU2swp(void*,int,T) const;
    void		putU4swp(void*,int,T) const;
    void		putU8swp(void*,int,T) const;
    void		putFswp(void*,int,T) const;
    void		putDswp(void*,int,T) const;

    void		putS2Ibmswp(void*,int,T) const;
    void		putS4Ibmswp(void*,int,T) const;
    void		putFIbmswp(void*,int,T) const;

    typedef T (DataInterpreter<T>::*GetFn)(const void*,int) const;
    typedef void (DataInterpreter<T>::*PutFn)(void*,int,T) const;
    typedef void (DataInterpreter<T>::*SwapFn)(void*,int) const;
    GetFn		getfn;
    PutFn		putfn;
    SwapFn		swpfn;

    void		swap0(void*,int) const		{}
    T			get0(const void*,int) const	{ return 0; }
    void		put0(void*,int,T) const		{}
    void		swpSwap();

};


#endif
