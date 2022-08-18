#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
class od_istream;

class SharedLibAccess;

/*!\brief OpendTect*/

namespace OD
{

/*!
\brief Module Dependency
*/

mExpClass(Basic) ModDep
{
public:
			ModDep( const char* m )
			    : name_(m)	{}
    bool		operator ==( const char* s ) const
			    { return name_ == s; }

    BufferString	name_;
    BufferStringSet	mods_;

};


/*!
\brief Dependency manager - provides tools for the dependencies between the
different OpendTect 'modules'.
*/

mExpClass(Basic) ModDepMgr
{
public:
				ModDepMgr(const char* fnm=0);
				~ModDepMgr();

    const ModDep*		find(const char*) const;
    const ObjectSet<ModDep>&	deps() const	{ return deps_; }

    void			ensureLoaded(const char*) const;
    const SharedLibAccess*	shLibAccess(const char*) const;

protected:

    ManagedObjectSet<ModDep>		deps_;

    mutable BufferStringSet		loadedmods_;
    mutable ObjectSet<SharedLibAccess>	shlibaccs_;

    void		readDeps(od_istream&);
    int			getLoadIdx(const char*) const;

public:
    void		closeAll();

};

mGlobal(Basic) const ModDepMgr& ModDeps();

#define mDefModInitFn(nm) \
mExternC(nm) void od_##nm##_initStdClasses(); \
extern "C" void od_##nm##_initStdClasses()


} // namespace OD
