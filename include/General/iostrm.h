#ifndef iostrm_H
#define iostrm_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2-8-1995
 RCS:		$Id: iostrm.h,v 1.30 2012-08-03 13:00:23 cvskris Exp $
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
    bool		isCommand() const		{ return iscomm; }

    void		copyFrom(const IOObj*);
    const char*		fullUserExpr(bool forread=true) const;
    const char*		getExpandedName(bool forread,
	    				bool fillwildcard=true) const;
    void		genDefaultImpl()		{ genFileName(); }

    const char*		connType() const;
    Conn*		getConn(Conn::State) const;

    bool		implExists(bool forread) const;
    bool		implReadOnly() const;
    bool		implRemove() const;
    bool		implShouldRemove() const;
    bool		implSetReadOnly(bool) const;
    bool		implRename(const char*,const CallBack* cb=0);

    bool		multiConn() const
			{ return isMulti() && curfnr <= fnrs.stop; }
    int			connNr() const
			{ return curfnr; }
    bool		toNextConnNr()
			{ curfnr += fnrs.step; return validNr(); }
    int			lastConnNr() const
			{ return fnrs.stop; }
    int			nextConnNr() const
			{ return curfnr+fnrs.step; }
    void		resetConnNr()
			{ curfnr = fnrs.start; }
    void		setConnNr( int nr )
			{ curfnr = nr; }

    const char*		hostName() const		{ return hostname; }
    void		setHostName( const char* hn )	{ hostname = hn; }
    const char*		fileName() const		{ return fname; }
    const char*		subDirName() const		{ return dirName(); }
    const char*		fullDirName() const;
    void		setFileName(const char*);
    void		setExt( const char* ext )	{ extension = ext; }
    void		genFileName();

    const char*		reader() const			{ return fname; }
    const char*		writer() const
			{ return writecmd ? (const char*)(*writecmd) : 0; }
    void		setReader(const char*);
    void		setWriter(const char*);

    int			zeroPadding() const		{ return padzeros; }
    void		setZeroPadding( int zp )	{ padzeros = zp; }
    StepInterval<int>&	fileNumbers()			{ return fnrs; }
    const StepInterval<int>& fileNumbers() const	{ return fnrs; }

    StreamProvider*	streamProvider(bool,bool fillwc=true) const;
    bool		isMulti() const
			{ return fnrs.start != fnrs.stop; }

protected:

    bool		getFrom(ascistream&);
    bool		putTo(ascostream&) const;

    BufferString	hostname;
    int			nrfiles;
    FileNameString	fname;

    BufferString	extension;
    FileNameString*	readcmd;
    FileNameString*	writecmd;
    bool		iscomm;
    int			padzeros;
    StepInterval<int>	fnrs;
    int			curfnr;
    int			nrretries;
    int			retrydelay;

    void		getDev(ascistream&);
    bool		validNr() const
			{ return curfnr*fnrs.step <= fnrs.stop*fnrs.step; }
    bool		implDo(bool,bool) const;

    static int		prodid; //!< for factory implementation
};


#endif

