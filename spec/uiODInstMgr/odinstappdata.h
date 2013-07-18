#ifndef appdata_h
#define appdata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
 RCS:           $Id: odinstappdata.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "odinstrel.h"
#include "odinstpkgkey.h"
class BufferStringSet;


namespace ODInst
{

class Version;


/*!\brief Set of installed packages */

mDefClass(uiODInstMgr) InstalledPkgSet : public ObjectSet<PkgKey>
{
public:

    int			getIndexOf(const char* pkgnm,Platform) const;
    const PkgKey*	get( const char* pkgnm, Platform plf ) const
			{ int idxof = getIndexOf(pkgnm,plf);
			  return idxof < 0 ? 0 : (*this)[idxof]; }

protected:

    friend class	AppData;
			
    bool		readFileNames(const char*,int,BufferStringSet&) const;
    			//!< Reads 'files.xx.txt' file to obtain package files
    BufferString	pkgFileName(int,bool) const;

};


/*!\brief Access to installed OpendTect app */

mDefClass(uiODInstMgr) AppData
{
public:

			AppData(const char* basedirnm=0,const char* reldirnm=0);

    bool		set(const char* basedirnm,const char* reldirnm);
    void		setRelType(RelType rt)		{ reltype_ = rt; }
    bool		setUpdateMode();
    const char*		errMsg() const			{ return errmsg_; }

    bool		exists() const			{ return exists_; }
    bool		isWritable() const		{ return exists_; }
    RelType		relType() const			{ return reltype_; }
    const char*		baseDirName() const		{ return basedir_; }
    const char*		dirName() const;
    const char*		fullDirName() const		{ return fulldirnm_; }
    BufferString	relInfoDirName() const;
    const InstalledPkgSet& installedPkgs() const	{ return pkgs_; }
    BufferString	getReport() const;
    BufferString	binPlfDirName() const;
    BufferString	binPlfBaseDirName() const;

    bool		isNewInst() const		{ return !exists(); }
    bool		getRunningProgs(BufferStringSet& list) const;
    bool		getFileList(const char* pkgnm,BufferString&) const;

    static const char*	sKeyRelInfoSubDir();
    static const char*	sKeyUpdateDir();

protected:

    BufferString	basedir_;
    BufferString	fulldirnm_;

    RelType		reltype_;
    bool		exists_;
    bool		writable_;
    InstalledPkgSet	pkgs_;
    Platform		platform_;

    mutable BufferString errmsg_;

    void		getInstalledPkgs();

public:

    bool		needUpdate(const BufferStringSet&,Platform,
	    			   BufferStringSet* pkgnms=0) const;

};


} // namespace

#endif

