#ifndef iostrm_H
#define iostrm_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		2-8-1995
 RCS:		$Id: iostrm.h,v 1.2 2000-01-24 16:34:57 bert Exp $
________________________________________________________________________

@$*/
 
/*@+
@$*/
 
#include <ioobject.h>
#include <conn.h>
#include <ranges.h>
class StreamProvider;


/*$@ IOStream
 is a file (default), device or command entry in the omf.
@$*/
class IOStream : public IOObject
{		 isUidConcreteDefObject(IOStream)

    friend class	dIOStream;

public:
			IOStream(const char* nm=0,const char* id=0,bool =0);
    virtual		~IOStream();
    bool		bad() const;
    StreamConn::Type	type() const			{ return type_; }

    void		copyFrom(const IOObj*);
    const char*		fullUserExpr(bool) const;
    const char*		hostName() const		{ return hostname; }
    void		setHostName( const char* hn )	{ hostname = hn; }
    void		genDefaultImpl()		{ genFileName(); }

    bool		implExists(bool forread) const;
    bool		implRemovable() const;
    bool		implRemove() const;

    const ClassDef&	connType() const;
    bool		multiConn() const
			{ return isMulti() && curfnr <= fnrs.stop; }
    Conn*		getConn(Conn::State) const;
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
    bool		isStarConn() const;

    const char*		fileName() const		{ return fname; }
    void		setFileName(const char*);
    void		setExt( const char* ext )	{ extension = ext; }
    void		genFileName();

    const char*		reader() const			{ return fname; }
    const char*		writer() const
			{ return writecmd ? (const char*)(*writecmd) : 0; }
    void		setReader(const char*);
    void		setWriter(const char*);

    const char*		devName() const;
    void		setDevName(const char*);
    int			blockSize() const		{ return blocksize; }
    void		setBlockSize( int bs )		{ blocksize = bs; }
    int			skipFiles() const		{ return skipfiles; }
    void		setSkipFiles( int sf )		{ skipfiles = sf; }
    void		setRewind( bool yn )		{ rew = yn; }
    bool		rewindTape() const		{ return rew; }

    int			zeroPadding() const		{ return padzeros; }
    void		setZeroPadding( int zp )	{ padzeros = zp; }
    StepInterval<int>&	fileNumbers()			{ return fnrs; }

protected:

    int			getFrom(ascistream&);
    int			putTo(ascostream&) const;

    StreamProvider*	streamProvider(bool) const;

    FixedString<32>	hostname;
    int			nrfiles;
    FileNameString	fname;

    FixedString<7>	extension;
    FileNameString*	readcmd;
    FileNameString*	writecmd;
    int			blocksize;
    int			skipfiles;
    bool		rew;
    int			padzeros;
    StepInterval<int>	fnrs;
    int			curfnr;

    StreamConn::Type	type_;

    bool		getDev(ascistream&);
    bool		isMulti() const
			{ return fnrs.start != fnrs.stop; }
    bool		validNr() const
			{ return curfnr*fnrs.step <= fnrs.stop*fnrs.step; }
};


#endif
