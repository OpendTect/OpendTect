#ifndef iostrm_H
#define iostrm_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		2-8-1995
 RCS:		$Id: iostrm.h,v 1.1.1.2 1999-09-16 09:20:04 arend Exp $
________________________________________________________________________

@$*/
 
/*@+
@$*/
 
#include <ioobject.h>
#include <strmprov.h>
#include <ranges.h>


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

    void		copyFrom(const IOObj*);
    const char*		fullUserExpr(bool) const;
    const char*		hostName() const		{ return hostname; }
    void		setHostName( const char* hn )	{ hostname = hn; }
    void		genDefaultImpl()		{ genFileName(); }

    bool		implExists(bool forread) const;
    bool		implRemovable() const;
    bool		implRemove() const;

    bool		multiConn() const
			{ return ismulti && curidx <= fnrs.stop; }
    Conn*		conn(Conn::State) const;
    Conn*		nextConn(Conn::State) const;
    void		skipConn() const;
    int			connNr() const			{ return curidx; }
    bool		isPercConn() const;

    StreamConn::Type	type() const			{ return type_; }

    const char*		fileName() const;
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

    void		setMulti( bool im )		{ ismulti = im; }
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
    bool		ismulti;
    int			padzeros;
    StepInterval<int>	fnrs;
    int			curidx;

    StreamConn::Type	type_;

    bool		getDev(ascistream&);
};


#endif
