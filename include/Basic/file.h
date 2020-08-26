#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 Contents:	File utitlities
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"

class BufferStringSet;
class Executor;
class uiString;


/*!\brief Interface for several file and directory related services */

namespace File
{

mGlobal(Basic) bool		exists(const char*);
mGlobal(Basic) bool		isReadable(const char*);
mGlobal(Basic) bool		isEmpty(const char*);
mGlobal(Basic) bool		isDirEmpty(const char*);
mGlobal(Basic) bool		isFile(const char*);
mGlobal(Basic) bool		isDirectory(const char*);
mGlobal(Basic) bool		isURI(const char*&);
				//!< if local, will remove 'file://'
mGlobal(Basic) BufferString	findExecutable(const char* exenm,
					       const BufferStringSet& paths,
					       bool includesyspath=true );

mGlobal(Basic) const char*	getCanonicalPath(const char*);
mGlobal(Basic) const char*	getRelativePath(const char* reltodir,
					const char* fnm);
mGlobal(Basic) void		makeRecursiveFileList(const char* dir,
					BufferStringSet& filelist,
					bool followlinks=false);

mGlobal(Basic) bool		createLink(const char* from,const char* to);
mGlobal(Basic) bool		isLink(const char*);
mGlobal(Basic) const char*	linkTarget(const char* linkname);
				//!< Direct, first target
mGlobal(Basic) const char*	linkEnd(const char* linkname);
				//!< 'Final' destination of link
				//!< protected against circular links
mGlobal(Basic) const char*	linkValue(const char* linkname);
				//!< not sure why and when to use - used by zip

mGlobal(Basic) void		hide(const char*,bool yn); //!<only Windows
mGlobal(Basic) bool		isHidden(const char*);
mGlobal(Basic) bool		isWritable(const char*);
mGlobal(Basic) bool		makeWritable(const char*,bool yesno,
					bool recursive);
mGlobal(Basic) bool		makeReadOnly(const char* fnm,bool recursive);
mGlobal(Basic) bool		isExecutable(const char*);
mGlobal(Basic) bool		makeExecutable(const char*,bool yesno);
mGlobal(Basic) bool		setPermissions(const char*,const char* perms,
					bool recursive);
mGlobal(Basic) bool		isInUse(const char* fnm);

enum DirListType		{ AllEntriesInDir, FilesInDir, DirsInDir };
mGlobal(Basic) bool		listDir(const char*,DirListType,
				    BufferStringSet& fnames,const char* mask=0);
mGlobal(Basic) bool		createDir(const char*);
mGlobal(Basic) bool		rename(const char* oldname,const char* newname);
mGlobal(Basic) bool		copy(const char* from,const char* to,
				     uiString* errmsg=0);
mGlobal(Basic) Executor*	getRecursiveCopier(const char* from,
					       const char* to);
mGlobal(Basic) Executor*	getRecursiveDeleter(const char* dirnm,
					const BufferStringSet* externallist=0,
					bool filesonly=false);
mGlobal(Basic) bool		resize(const char*,od_int64);
mGlobal(Basic) bool		remove(const char*);
mGlobal(Basic) bool		saveCopy(const char* from,const char* to);
mGlobal(Basic) bool		copyDir(const char* from,const char* to,
					uiString* errmsg=0);
mGlobal(Basic) bool		removeDir(const char*);
mGlobal(Basic) bool		changeDir(const char* path);
mGlobal(Basic) bool		checkDirectory(const char* fnm,bool forread,
					       uiString& errmsg);
				/*!< checks if the parent directory of a file
				     is readable/writable */

mGlobal(Basic) bool		getContent(const char*,BufferString&);
mGlobal(Basic) od_int64		getFileSize(const char* fnm,
					bool followlink=true); //!< bytes
mGlobal(Basic) od_int64		getKbSize(const char*);
mGlobal(Basic) BufferString	getFileSizeString(od_int64 fileszinkb);
mGlobal(Basic) BufferString	getFileSizeString(const char* fnm);

mGlobal(Basic) const char*	timeCreated(const char*);	//!< ISO/UTC
mGlobal(Basic) const char*	timeLastModified(const char*);	//!< ISO/UTC
mGlobal(Basic) od_int64		getTimeInSeconds(const char*,bool modif=true);
mGlobal(Basic) od_int64		getTimeInMilliSeconds(const char* fnm,
					bool lastmodif=true);//since day start
mGlobal(Basic) bool		waitUntilExists(const char*,double maxwaittm,
						double* actualwaited=nullptr);

mGlobal(Basic) const char*	getCurrentPath();
mGlobal(Basic) const char*	getHomePath();
mGlobal(Basic) const char*	getTempPath();
mGlobal(Basic) const char*	getRootPath(const char* path);

					/* Internal use only: */
mGlobal(Basic) bool			initTempDir();


} // namespace File
