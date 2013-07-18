#ifndef odinstpkgselmgr_h
#define odinstpkgselmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2011
 RCS:           $Id: odinstpkgselmgr.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "bufstringset.h"
#include "odinstver.h"
#include "objectset.h"
#include "enums.h"


namespace ODInst
{

class AppData;
class Platform;
class PkgProps;
class PkgGroupSet;

/* Selection Manager, only cares about packages on one platform */

mDefClass(uiODInstMgr) PkgSelection
{
public:
    			PkgSelection( const PkgProps& pp, bool ii,
				      const Version& v )
			    : pp_(pp), isinst_(ii), instver_(v)
			    , doforceinstall_(false)
			{ issel_ = isinst_; }

    const PkgProps&	pp_;
    const bool		isinst_;
    const Version	instver_;

    bool		issel_;
    bool		doforceinstall_;
};


mDefClass(uiODInstMgr) PkgSelMgr : public ObjectSet<PkgSelection>
{
public:

    			PkgSelMgr(const AppData&,const PkgGroupSet&,
				  const Platform&);
			~PkgSelMgr()	{ deepErase(*this); }

    enum ReqAction	{ None, Install, Upgrade, Remove, Reinstall };
    			// Reinstall does a force install when no update needed
    			DeclareEnumUtils(ReqAction)

    int			idxOf(const PkgProps&) const;
    int			idxOf( const char* pkgnm ) const;

    int			nrSelections() const;
    bool		isNeeded(ReqAction) const;
    int			nrFiles2Get(bool include_hidden=true) const;

    bool		isSelected(const PkgProps&) const;
    bool		isInstalled(const PkgProps&) const;
    Version		version(const PkgProps&,bool installed) const;
    ReqAction		reqAction(const PkgProps&) const;
    ReqAction		reqAction(const PkgSelection&) const;

    void		setSelected(const PkgProps&,bool);
    void		setForceReinstall(const PkgProps&,bool);
    bool		shouldReInstall(const PkgProps&) const;
    void		getPackageListsForAction(BufferStringSet& todownload,
					         BufferStringSet* installed=0,
						 BufferStringSet* updated=0,
						BufferStringSet* reinstalled=0);
    BufferString	getFullPackageName(const BufferString& pkgnm) const;
    BufferString	getUserName(const BufferString& pkgnm) const;
    void		getOfflineInstallPackages(BufferStringSet&);
protected:

    const Platform&	platform_;
};

} // namespace

#endif

