#ifndef moddepmgr_h
#define moddepmgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2011
 RCS:		$Id: moddepmgr.h,v 1.4 2012-08-03 13:00:13 cvskris Exp $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
class SharedLibAccess;


/*!\brief Dependency manager - provides tools for the dependencies between
          the different OpendTect 'modules'.
 */

namespace OD
{

mClass(Basic) ModDep
{
public:
    			ModDep( const char* m )
			    : name_(m)	{}
    bool		operator ==( const char* s ) const
			    { return name_ == s; }

    BufferString	name_;
    BufferStringSet	mods_;

};

mClass(Basic) ModDepMgr
{
public:
				ModDepMgr(const char* fnm=0);

    const ModDep*		find(const char*) const;
    const ObjectSet<ModDep>&	deps() const	{ return deps_; }

    void			ensureLoaded(const char*) const;
    const SharedLibAccess*	shLibAccess(const char*) const;

protected:

    ObjectSet<ModDep>	deps_;
    bool		isrel_;
    BufferString	relbindir_;
    BufferString	devbindir_;

    mutable BufferStringSet		loadedmods_;
    mutable ObjectSet<SharedLibAccess>	shlibaccs_;

    void		readDeps(std::istream&);
    int			getLoadIdx(const char*) const;

};

mGlobal(Basic) const ModDepMgr& ModDeps();

#define mDefModInitFn(nm) \
    mExternC(Basic) void od_##nm##_initStdClasses(); \
extern "C" void od_##nm##_initStdClasses()


} // namespace OD

#endif

