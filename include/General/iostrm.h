#ifndef iostrm_H
#define iostrm_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2-8-1995
 RCS:		$Id$
________________________________________________________________________


-*/
 
#include "generalmod.h"
#include "ioobj.h"
#include "streamconn.h"
#include "ranges.h"
class StreamProvider;

/*\brief An IOStream is a file (default) or command entry in the omf. */


mClass(General) IOStream : public IOObj
{
public:
			IOStream(const char* nm=0,const char* id=0,
				 bool =false);
    virtual		~IOStream();
    bool		bad() const;
    bool		isCommand() const		{ return iscomm_; }

    void		copyFrom(const IOObj*);
    const char*		fullUserExpr(bool forread=true) const;
    const char*		getExpandedName(bool forread,
	    				bool fillwildcard=true) const;
    void		genDefaultImpl()		{ genFileName(); }

    FixedString		connType() const;
    Conn*		getConn(Conn::State) const;

    bool		implExists(bool forread) const;
    bool		implReadOnly() const;
    bool		implRemove() const;
    bool		implShouldRemove() const;
    bool		implSetReadOnly(bool) const;
    bool		implRename(const char*,const CallBack* cb=0);

    bool		multiConn() const
			{ return isMulti() && curfnr_ <= fnrs_.stop; }
    int			connNr() const
			{ return curfnr_; }
    bool		toNextConnNr()
			{ curfnr_ += fnrs_.step; return validNr(); }
    int			lastConnNr() const
			{ return fnrs_.stop; }
    int			nextConnNr() const
			{ return curfnr_+fnrs_.step; }
    void		resetConnNr()
			{ curfnr_ = fnrs_.start; }
    void		setConnNr( int nr )
			{ curfnr_ = nr; }

    const char*		hostName() const		{ return hostname_; }
    void		setHostName( const char* hn )	{ hostname_ = hn; }
    FixedString		fileName() const		{ return fname_.buf(); }
    const char*		subDirName() const		{ return dirName(); }
    const char*		fullDirName() const;
    void		setFileName(const char*);
    void		setExt( const char* ext )	{ extension_ = ext; }
    void		genFileName();

    const char*		reader() const			{ return fname_; }
    const char*		writer() const
			{ return writecmd_ ? (const char*)(*writecmd_) : 0; }
    void		setReader(const char*);
    void		setWriter(const char*);

    int			zeroPadding() const		{ return padzeros_; }
    void		setZeroPadding( int zp )	{ padzeros_ = zp; }
    StepInterval<int>&	fileNumbers()			{ return fnrs_; }
    const StepInterval<int>& fileNumbers() const	{ return fnrs_; }

    StreamProvider*	streamProvider(bool,bool fillwc=true) const;
    bool		isMulti() const
			{ return fnrs_.start != fnrs_.stop; }

protected:

    bool		getFrom(ascistream&);
    bool		putTo(ascostream&) const;

    BufferString	hostname_;
    int			nrfiles_;
    FileNameString	fname_;

    BufferString	extension_;
    FileNameString*	readcmd_;
    FileNameString*	writecmd_;
    bool		iscomm_;
    int			padzeros_;
    StepInterval<int>	fnrs_;
    int			curfnr_;
    int			nrretries_;
    int			retrydelay_;

    void		getDev(ascistream&);
    bool		validNr() const
			{ return curfnr_*fnrs_.step <= fnrs_.stop*fnrs_.step; }
    bool		implDo(bool,bool) const;

    static int		prodid_; //!< for factory implementation
};


#endif

