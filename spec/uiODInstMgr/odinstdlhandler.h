#ifndef odinstdlhandler_h
#define odinstdlhandler_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
 RCS:           $Id: odinstdlhandler.h 8009 2013-06-20 06:11:26Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

#include "uiodinstmgrmod.h"
#include "bufstringset.h"
#include "callback.h"
class ODDLSite;
class TaskRunner;
class DataBuffer;


namespace ODInst
{

/* gives access to the opendtect.org download sites/mirrors */


mDefClass(uiODInstMgr) DLHandler : public CallBacker
{
public:

    mDefClass(uiODInstMgr) FailHndlr
    {
    public:

	virtual		~FailHndlr()		{}

	virtual bool	handle(ODDLSite&,BufferString& sitenm,float& tmout) = 0;

	bool		isfatal_;

    };

			DLHandler(const DataBuffer& sites,
				  const char* subdir=0,float tmout=0);
			~DLHandler();

    FailHndlr*		failHandler()			{ return failhndlr_; }
    void		setFailHandler(FailHndlr*);	//!< stays yours
    const ODDLSite&	site() const			{ return *dlsite_; }
    bool		reConnectSite();
    void		setSite(const char*,float tmout=0);
    BufferString	fullURL(const char* reldirnm) const;

    bool		remoteStatusOK(bool checkprev=true);

    bool		fetchFile(const char*,const char* outfnm=0,
					TaskRunner* tr=0,
					const char* nicename=0);
    const DataBuffer&	fileData() const;

    bool		isOK() const;
    int			remoteStatus() const	{ return lastremotestatus_; }
    const char*		errMsg() const; //!< after isOK() or fetchFile()

    bool		fetchFiles(const BufferStringSet&,const char* outdir,
	    			   TaskRunner&);
    			//!< check TaskRunner for status/errmsg

    const BufferStringSet& availableSites() const	{ return avsites_; }

    static void		getFileData(const DataBuffer&,BufferStringSet&,
	    			    int maxlen=1024);
    static DataBuffer*	getSites(const BufferStringSet&,FailHndlr&,
	    			 float& tmout);
    static const BufferStringSet& seedSites();

protected:

    ODDLSite*		dlsite_;
    FailHndlr*		failhndlr_;
    BufferStringSet	avsites_;
    mutable DataBuffer*	databuf_;
    int			lastremotestatus_;

    bool		useFailHandler();

};


} // namespace

#endif

