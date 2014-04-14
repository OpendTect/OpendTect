#ifndef file_h
#define file_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 Contents:	File utitlities
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "gendefs.h"
#include "timefun.h"

class BufferStringSet;
class Executor;


/*!\brief Interface for several file and directory related services */

namespace File
{

mGlobal(Basic) bool		exists(const char*);
mGlobal(Basic) bool		isEmpty(const char*);
mGlobal(Basic) bool		isFile(const char*);
mGlobal(Basic) bool		isDirectory(const char*);

mGlobal(Basic) const char*	getCanonicalPath(const char*);
mGlobal(Basic) const char*	getRelativePath(const char* reltodir,
					const char* fnm);
mGlobal(Basic) void		makeRecursiveFileList(const char* dir,
					BufferStringSet& filelist,
					bool followlinks=false);

mGlobal(Basic) bool		createLink(const char* from,const char* to);
mGlobal(Basic) bool		isLink(const char*);
mGlobal(Basic) const char*	linkTarget(const char* linkname);
mGlobal(Basic) const char*	linkValue(const char* linkname);

mGlobal(Basic) bool		isHidden(const char*);
mGlobal(Basic) bool		isWritable(const char*);
mGlobal(Basic) bool		makeWritable(const char*,bool yesno,
					bool recursive);
mGlobal(Basic) bool		isExecutable(const char*);
mGlobal(Basic) bool		makeExecutable(const char*,bool yesno);
mGlobal(Basic) bool		setPermissions(const char*,const char* perms,
					bool recursive);
mGlobal(Basic) bool		isFileInUse(const char* fnm);

mGlobal(Basic) bool		createDir(const char*);
mGlobal(Basic) bool		rename(const char* oldname,const char* newname);
mGlobal(Basic) bool		copy(const char* from,const char* to,
					BufferString* errmsg=0);
mGlobal(Basic) Executor*	getRecursiveCopier(const char* from,
					       const char* to);
mGlobal(Basic) bool		remove(const char*);
mGlobal(Basic) bool		saveCopy(const char* from,const char* to);
mGlobal(Basic) bool		copyDir(const char* from,const char* to,
					BufferString* errmsg=0);
mGlobal(Basic) bool		removeDir(const char*);
mGlobal(Basic) bool		changeDir(const char* path);

mGlobal(Basic) bool		getContent(const char*,BufferString&);
mGlobal(Basic) od_int64		getFileSize(const char* fnm,
					bool followlink=true); //!< bytes
mGlobal(Basic) int		getKbSize(const char*);

mGlobal(Basic) const char*	timeCreated(const char* filenm,
					const char* fmt=Time::defDateTimeFmt());
mGlobal(Basic) const char*	timeLastModified(const char* filenm,
					const char* fmt=Time::defDateTimeFmt());
mGlobal(Basic) od_int64		getTimeInSeconds(const char*,bool modif=true);

mGlobal(Basic) const char*	getCurrentPath();
mGlobal(Basic) const char*	getHomePath();
mGlobal(Basic) const char*	getTempPath();
mGlobal(Basic) const char*	getRootPath(const char* path);


enum ViewStyle			{ Text, Table, Log, Bin };

mClass(Basic) ViewPars
{
public:

				ViewPars( ViewStyle vs=Text )
				    : style_(vs)
				    , editable_(false)
				    , maxnrlines_(vs==Table?500:10000)	{}

    ViewStyle			style_;
    int				maxnrlines_;	//!< max nr bytes when Bin
    bool			editable_;	//!< only if possible

};

mGlobal(Basic) bool		launchViewer(const char*,
					const ViewPars& vp=ViewPars());


} // namespace File


#endif
