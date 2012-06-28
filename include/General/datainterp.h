#ifndef datainterp_h
#define datainterp_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Nov 2000
 Contents:	Binary data interpretation
 RCS:		$Id: datainterp.h,v 1.17 2012-06-28 12:59:27 cvskris Exp $
________________________________________________________________________

*/


#include "datachar.h"
#include "general.h"

class IOPar;


/*!\brief Byte-level data interpreter.

Efficient (one function call overhead) get and set of data, usually in a data
buffer. Facility to swap bytes in advance. The interpretation is into/from
the template parameter. At present, float, double, int and long long
are supported and instantiated.

*/

#if defined(__msvc__) && (defined(GENERAL_EXPORTS) || defined(General_EXPORTS) )
# define mGenClass	class dll_export
#else
# define mGenClass	class
#endif

template<class T>
mGenClass DataInterpreter
{
public:
    static DataInterpreter<T>	create();
    static DataInterpreter<T>*	create(const DataCharacteristics&,
				       bool alsoifequal);
				/*!<\param alsoifequal determines whether an
				           interpreter should be created
					   if the format in DataChar is
					   identical to the current machine's.*/
    static DataInterpreter<T>*	create(const char*,bool alsoifequal);
				/*!<\param alsoifequal determines whether an
				           interpreter should be created
					   if the format in DataChar is
					   identical to the current machine's.*/
    static DataInterpreter<T>*	create(const IOPar& par,const char* key,
				       bool alsoifequal);
				/*!<\param alsoifequal determines whether an
				           interpreter should be created
					   if the format in DataChar is
					   identical to the current machine's.*/

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
    void		swap( void* buf, od_int64 bufsz_in_elements ) const
			{ (this->*swpfn)( buf, bufsz_in_elements );
			  const_cast<DataInterpreter<T>*>(this)->swpSwap(); }

    T			get(std::istream&) const;
    static T		get(const DataInterpreter<T>*,std::istream&);
    static bool         get(const DataInterpreter<T>*,std::istream&,T&);
    inline T		get( const void* buf, od_int64 nr ) const
			{ return (this->*getfn)( buf, nr ); }
    inline void		put( void* buf, od_int64 nr, T t ) const
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

    void		swap2(void*,od_int64) const;
    void		swap4(void*,od_int64) const;
    void		swap8(void*,od_int64) const;

    T			getS1(const void*,od_int64) const;
    T			getS2(const void*,od_int64) const;
    T			getS4(const void*,od_int64) const;
    T			getS8(const void*,od_int64) const;
    T			getU1(const void*,od_int64) const;
    T			getU2(const void*,od_int64) const;
    T			getU4(const void*,od_int64) const;
    T			getF(const void*,od_int64) const;
    T			getD(const void*,od_int64) const;

    T			getS2Ibm(const void*,od_int64) const;
    T			getS4Ibm(const void*,od_int64) const;
    T			getFIbm(const void*,od_int64) const;

    T			getS2swp(const void*,od_int64) const;
    T			getS4swp(const void*,od_int64) const;
    T			getS8swp(const void*,od_int64) const;
    T			getU2swp(const void*,od_int64) const;
    T			getU4swp(const void*,od_int64) const;
    T			getFswp(const void*,od_int64) const;
    T			getDswp(const void*,od_int64) const;

    T			getS2Ibmswp(const void*,od_int64) const;
    T			getS4Ibmswp(const void*,od_int64) const;
    T			getFIbmswp(const void*,od_int64) const;

    void		putS1(void*,od_int64,T) const;
    void		putS2(void*,od_int64,T) const;
    void		putS4(void*,od_int64,T) const;
    void		putS8(void*,od_int64,T) const;
    void		putU1(void*,od_int64,T) const;
    void		putU2(void*,od_int64,T) const;
    void		putU4(void*,od_int64,T) const;
    void		putF(void*,od_int64,T) const;
    void		putD(void*,od_int64,T) const;

    void		putS2Ibm(void*,od_int64,T) const;
    void		putS4Ibm(void*,od_int64,T) const;
    void		putFIbm(void*,od_int64,T) const;

    void		putS2swp(void*,od_int64,T) const;
    void		putS4swp(void*,od_int64,T) const;
    void		putS8swp(void*,od_int64,T) const;
    void		putU2swp(void*,od_int64,T) const;
    void		putU4swp(void*,od_int64,T) const;
    void		putFswp(void*,od_int64,T) const;
    void		putDswp(void*,od_int64,T) const;

    void		putS2Ibmswp(void*,od_int64,T) const;
    void		putS4Ibmswp(void*,od_int64,T) const;
    void		putFIbmswp(void*,od_int64,T) const;

    typedef T (DataInterpreter<T>::*GetFn)(const void*,od_int64) const;
    typedef void (DataInterpreter<T>::*PutFn)(void*,od_int64,T) const;
    typedef void (DataInterpreter<T>::*SwapFn)(void*,od_int64) const;
    GetFn		getfn;
    PutFn		putfn;
    SwapFn		swpfn;

    void		swap0(void*,od_int64) const		{}
    T			get0(const void*,od_int64) const	{ return 0; }
    void		put0(void*,od_int64,T) const		{}
    void		swpSwap();

};


#endif
