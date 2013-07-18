#ifndef odinstinstallhandler_h
#define odinstinstallhandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay
 Date:          May 2012
 RCS:           $Id: odinstinstallhandler.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "odinstappdata.h"
#include "odinstver.h"

class TaskRunner;

namespace ODInst
{

class AppData;
class DLHandler;
class PkgSelMgr;
class Platform;


/*!\brief This is the main installer, basically
 
It:

  * Sets up the 'transaction environment'
  	- Copy current to 'update'

  * Downloads and unzips the packages (works in 'update')

  * Finalises the transaction
  	- Rename current to 'old'
	- Rename 'update' to current

  * [Win] finalise (creates icon etc)
 
 */


class InstallHandler : public CallBacker
{
public:
			InstallHandler(AppData&,const RelData& rd,
					const Platform&,PkgSelMgr&,
					DLHandler&);
			~InstallHandler();
    bool		downLoadPackages(TaskRunner*,bool isofl=false);
    bool		installZipPackages(bool isoffline,TaskRunner*);
    bool		configureInstallation();
    bool		isNewInst() const { return isnewinst_; }
   
    DLHandler&		dlHandler()	    { return dlhandler_; }
    BufferString	errorMsg() const    { return errmsg_; }

    const BufferStringSet&	installedPackages() const
    				{ return toinstallpkgs_; }
    const BufferStringSet&	updatedPackages() const
    				{ return updatedpkgs_; }
    const BufferStringSet&	reinstalledPackages() const
    				{ return reinstalledpkgs_; }
    const BufferStringSet&	downLoadedPackages() const
    				{ return downloadedpkgs_;}
    bool			copyWinOfflineInstallerFilesTo(const char*);
    
    CNotifier<InstallHandler,BufferString> status;

protected:

    bool		prepareForUpdate(TaskRunner* tr=0);
    bool		restoreUpdates();
    void		checkInstallerVersion() const;
    Version		getVersionFromPath(const BufferString& path) const;
    bool		prepareOfflineInstaller(TaskRunner*);
    bool		downloadSetupFiles(TaskRunner*);

    const AppData	oldappdata_;
    AppData&		appdata_;
    const Platform&	platform_;
    PkgSelMgr&		pkgselmgr_;
    const RelData&	reldata_;
    BufferStringSet	packagelist_;
    BufferStringSet	pckgstoinstall_;
    BufferStringSet	toinstallpkgs_;
    BufferStringSet	installedpkgs_;
    BufferStringSet	updatedpkgs_;
    BufferStringSet	reinstalledpkgs_;
    BufferStringSet	downloadedpkgs_;
    BufferStringSet	downloadedzipfiles_;

    BufferString	errmsg_;
    DLHandler&		dlhandler_;
    const bool		isnewinst_;
};

} // namespace ODInst;

#endif
