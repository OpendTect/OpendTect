#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2011
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
#include "od_iosfwd.h"

class SharedLibAccess;

/*!\brief OpendTect*/

namespace OD
{

/*!\brief Module Dependency */

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

    static const char*		sAllNonUI()	{ return "AllNonUi"; }
    static const char*		sAllUI()	{ return "uiODMain"; }

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
