#ifndef gpuprog_h
#define gpuprog_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Mar 2016
________________________________________________________________________

-*/

#include "bufstring.h"
#include "bufstring.h"
#include "objectset.h"
#include "refcount.h"

namespace GPU
{

mExpClass(Basic) Device
{
public:
    virtual			~Device() {}
    od_int64			totalmem_;
    od_int64			maxmemalloc_;

    int				max2Dsize_[2];
    int				max3Dsize_[3];

    bool			iscpu_;
    BufferString		name_;
};


mExpClass(Basic) Platform
{
public:
    static const ObjectSet<Platform>&	getPlatforms();

    BufferString                        vendor_;
    BufferString                        name_;

    ObjectSet<Device>			devices_;
};


mExpClass(Basic) Context : public RefCount::Referenced
{
public:
    static RefMan<Context> 		createContext( const Device& );
};


//For internal implementation only.
typedef Context* (*ContextCreateFunction)(const Device&);
mGlobal(Basic) void setContextCreatorFunction(ContextCreateFunction);
mGlobal(Basic) void setPlatforms(ObjectSet<Platform>&);



}  //namespace

#endif
