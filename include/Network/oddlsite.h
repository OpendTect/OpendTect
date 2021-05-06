#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
________________________________________________________________________

-*/

#include "networkmod.h"
#include "callback.h"
#include "bufstring.h"
#include "uistring.h"
class BufferStringSet;
class TaskRunner;
class DataBuffer;


/*!\brief Access to an http site to get the contents of files.
 
  The URL defaults to opendtect.org. The subdir is going to be the current
  work directory on the site. You can change it at any time (unlike the host).

  There are two main services: get one file, or get a bunch of files. To get
  one file, you request it. Set some kind of status bar, because it won't
  return until either an error occurs (e.g. a timeout is exceeded),
  or the file is there.  This is meant for small files and minimum hassle.

  If you need bigger files and/or you want users to be able to interrupt, then
  you need the version with the TaskRunner.

  You can ask for a plain file on a local disk; pass 'DIR=the_file_name'.
  Then, no HTTP connection is made. If the site needs secure access, pass the
  full URL (i.e. https://xx.yy). At the time I'm making this comment, it is
  not implemented. For normal HTTP access, you can pass the host name or
  http://hostname .

 */

mExpClass(Network) ODDLSite : public CallBacker
{ mODTextTranslationClass(ODDLSite);
public:

			ODDLSite(const char* the_host,float timeout_sec=0);
			~ODDLSite();
    bool		isOK() const			{ return !isfailed_; }

    const char*		host() const			{ return host_; }
    const char*		subDir() const			{ return subdir_; }
    void		setSubDir( const char* s )	{ subdir_ = s; }
    float		timeout() const			{ return timeout_; }
    void		setTimeOut(float,bool storeinsettings);

    uiString		errMsg() const			{ return errmsg_; }

    bool		getFile(const char* fnm,const char* outfnm=0,
				 TaskRunner* tr=0, const char* nicename=0);
    			//!< Without a file name, get the DataBuffer
    DataBuffer*		obtainResultBuf();
    			//!< the returned databuf becomes yours

    bool		getFiles(const BufferStringSet& fnms,
				  const char* outputdir,TaskRunner&);
    od_int64		getFileSize(const char* relfilenm);
    bool		haveErrMsg() const
    			{ return !errmsg_.isEmpty(); }
    			//!< if haveErrMsg(), then failure - otherwise user stop

    BufferString	fullURL(const char*) const;

protected:

    BufferString	host_;
    BufferString	subdir_;
    float		timeout_;
    bool		islocal_;
    bool		issecure_;

    mutable uiString	errmsg_;
    bool		isfailed_;
    DataBuffer*		databuf_;

    void		reqFinish(CallBacker*);
    BufferString	getFileName(const char*) const;
    bool		getLocalFile(const char*,const char*);
};



