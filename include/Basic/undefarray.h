#ifndef undefarray_h
#define undefarray_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          13/01/2005
 RCS:           $Id: undefarray.h,v 1.5 2010/01/22 18:51:04 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "commondefs.h"
#include "plftypes.h"
#include "ptrman.h"
#include "valseries.h"

class BinDataDesc;

/*! Filter out undefined values and replaces them by a linear interpolation
    of surrounding defined values. If undefined values are found at start/end,
    they are replaced by the first/last defined value.
    Input and output may be the same.
    \returns true if success, false if no defined value was found. */
template <class T> 
inline bool filterUndef(const T* input,T* output,int sz);

template <class T>
inline bool filterUndef(const ValueSeries<T>& input,ValueSeries<T>& output,int);


/*!Class that handles undefvalues in arrays that are in a format described
   by a BinDataDesc */

mClass UndefArrayHandler
{
public:
		UndefArrayHandler(const BinDataDesc& desc);
    bool	set(const BinDataDesc& desc);
    bool	isOK() const;

    bool	isUdf(const void* ptr, od_int64 idx) const;
    void	setUdf(void* ptr, od_int64 idx) const;
    void	unSetUdf(void* ptr, od_int64 idx) const;
    		/*!<If the value is undef, it is replaced by a similar value
		    that isn't undef. */

protected:
    		typedef	bool (*IsUdfFunc)(const void*,od_int64 idx);
    		typedef	void (*SetUdfFunc)(void*,od_int64 idx);
    		typedef	void (*UnsetUdfFunc)(void*,od_int64 idx);

    IsUdfFunc		isudf_;
    SetUdfFunc		setudf_;
    UnsetUdfFunc	limitrange_;

    static bool	isUdfUChar(const void*,od_int64);
    static void setUdfUChar(void*,od_int64);
    static void unsetUdfUChar(void*,od_int64);

    static bool	isUdfChar(const void*,od_int64);
    static void setUdfChar(void*,od_int64);
    static void unsetUdfChar(void*,od_int64);

    static bool	isUdfUShort(const void*,od_int64);
    static void setUdfUShort(void*,od_int64);
    static void unsetUdfUShort(void*,od_int64);

    static bool	isUdfShort(const void*,od_int64);
    static void setUdfShort(void*,od_int64);
    static void unsetUdfShort(void*,od_int64);

    static bool	isUdfUInt32(const void*,od_int64);
    static void setUdfUInt32(void*,od_int64);
    static void unsetUdfUInt32(void*,od_int64);

    static bool	isUdfInt32(const void*,od_int64);
    static void setUdfInt32(void*,od_int64);
    static void unsetUdfInt32(void*,od_int64);

    static bool	isUdfUInt64(const void*,od_int64);
    static void setUdfUInt64(void*,od_int64);
    static void unsetUdfUInt64(void*,od_int64);

    static bool	isUdfInt64(const void*,od_int64);
    static void setUdfInt64(void*,od_int64);
    static void unsetUdfInt64(void*,od_int64);

    static bool	isUdfFloat(const void*,od_int64);
    static void setUdfFloat(void*,od_int64);
    static void unsetUdfFloat(void*,od_int64);

    static bool	isUdfDouble(const void*,od_int64);
    static void setUdfDouble(void*,od_int64);
    static void unsetUdfDouble(void*,od_int64);
};


template <class T> inline
bool filterUndef( const ValueSeries<T>& input, ValueSeries<T>& output, int sz )
{
    if ( !sz ) return true;
    
    const T* inptr = input.arr();
    T* outptr = output.arr();
    
    ArrPtrMan<T> myinp = 0, myoutp = 0;
    if ( !inptr )
    {
	myinp = new T[sz];
	for ( int idx=0; idx<sz; idx++ )
	    myinp[idx] = input.value(idx);
    }
    
    if ( !outptr )
	myoutp = outptr = new T[sz];
    
    return filterUndef( inptr ? inptr : myinp, outptr, sz );
}


template <class T> inline
bool filterUndef(const T* input, T* output, int sz )
{
    if ( !sz )
	return true;
    
    int firstdefined = 0;
    while ( firstdefined<sz && mIsUdf(input[firstdefined]) )
	firstdefined++;
    
    if ( firstdefined==sz )
	return false;
    
    for ( int idx=0; idx<=firstdefined; idx++ )
	output[idx] = input[firstdefined];
    
    if ( firstdefined==sz-1 )
	return true;
    
    int prevdefined = firstdefined;
    int nextdefined = -1;
    
    for ( int idx=firstdefined+1; idx<sz; )
    {
	if ( !mIsUdf(input[idx]) )
	{
	    prevdefined = idx;
	    output[idx] = input[idx];
	    idx++;
	    
	    continue;		
	}

	nextdefined = idx+1;
	while ( nextdefined<sz && mIsUdf(input[nextdefined]) )
	    nextdefined++;

	idx = nextdefined;

	if ( nextdefined==sz )
	{
	    for ( int posidx = prevdefined+1; posidx<sz; posidx++ )
		output[posidx] = input[prevdefined];

	    return true;
	}
	else
	{
	    const T diff = input[nextdefined] - input[prevdefined];
	    const T unit = diff / (float)(nextdefined-prevdefined);
	    for ( int posidx = prevdefined+1; posidx<=nextdefined; posidx++ )
		output[posidx] = input[prevdefined]+unit*(posidx-prevdefined);	

	    prevdefined = nextdefined;
	    nextdefined = -1;
	}
    }

    return true;
}

#endif
