#ifndef undefarray_h
#define undefarray_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          13/01/2005
 RCS:           $Id: undefarray.h,v 1.4 2009-07-22 16:01:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "plftypes.h"
#include "commondefs.h"
class BinDataDesc;


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

#endif
