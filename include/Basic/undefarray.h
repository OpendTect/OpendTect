#ifndef undefarray_h
#define undefarray_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          13/01/2005
 RCS:           $Id: undefarray.h,v 1.1 2006-12-20 17:27:33 cvskris Exp $
________________________________________________________________________

-*/

#include "plftypes.h"

class BinDataDesc;


/*!Class that handles undefvalues in arrays that are in a format described
   by a BinDataDesc */

class UndefArrayHandler
{
public:
		UndefArrayHandler(const BinDataDesc& desc);
    bool	set(const BinDataDesc& desc);
    bool	isOK() const;

    bool	isUdf(const void* ptr, int64 idx) const;
    void	setUdf(void* ptr, int64 idx) const;
    void	unSetUdf(void* ptr, int64 idx) const;
    		/*!<If the value is undef, it is replaced by a similar value
		    that isn't undef. */

protected:
    		typedef	bool (*IsUdfFunc)(const void*,int64 idx);
    		typedef	void (*SetUdfFunc)(void*,int64 idx);
    		typedef	void (*UnsetUdfFunc)(void*,int64 idx);

    IsUdfFunc		isudf_;
    SetUdfFunc		setudf_;
    UnsetUdfFunc	limitrange_;

    static bool	isUdfUChar(const void*,int64);
    static void setUdfUChar(void*,int64);
    static void unsetUdfUChar(void*,int64);

    static bool	isUdfChar(const void*,int64);
    static void setUdfChar(void*,int64);
    static void unsetUdfChar(void*,int64);

    static bool	isUdfUShort(const void*,int64);
    static void setUdfUShort(void*,int64);
    static void unsetUdfUShort(void*,int64);

    static bool	isUdfShort(const void*,int64);
    static void setUdfShort(void*,int64);
    static void unsetUdfShort(void*,int64);

    static bool	isUdfUInt32(const void*,int64);
    static void setUdfUInt32(void*,int64);
    static void unsetUdfUInt32(void*,int64);

    static bool	isUdfInt32(const void*,int64);
    static void setUdfInt32(void*,int64);
    static void unsetUdfInt32(void*,int64);

    static bool	isUdfUInt64(const void*,int64);
    static void setUdfUInt64(void*,int64);
    static void unsetUdfUInt64(void*,int64);

    static bool	isUdfInt64(const void*,int64);
    static void setUdfInt64(void*,int64);
    static void unsetUdfInt64(void*,int64);

    static bool	isUdfFloat(const void*,int64);
    static void setUdfFloat(void*,int64);
    static void unsetUdfFloat(void*,int64);

    static bool	isUdfDouble(const void*,int64);
    static void setUdfDouble(void*,int64);
    static void unsetUdfDouble(void*,int64);
};

#endif
