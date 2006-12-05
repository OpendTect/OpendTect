#ifndef seisimporter_h
#define seisimporter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Nov 2006
 RCS:		$Id: seisimporter.h,v 1.2 2006-12-05 16:49:09 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "executor.h"
#include "bufstring.h"
class IOObj;
class SeisTrc;
class SeisTrcBuf;
class SeisTrcWriter;


/*!\brief Helps import or export of seismic data. */

class SeisImporter : public Executor
{
public:

    /*!<\brief provides traces from the import storage

      fetch() must return false at end or when an error occurs.
      On error, the errmsg_ must be filled.

      */

    struct Reader
    {
	virtual			~Reader()			{}

	virtual const char*	name() const			= 0;
	virtual bool		fetch(SeisTrc&)			= 0;
	virtual int		totalNr() const			{ return -1; }

	BufferString		errmsg_;
    };


    			SeisImporter(Reader*,SeisTrcWriter&,Seis::GeomType);
				//!< Reader becomes mine. Has to be non-null.
    virtual		~SeisImporter();

    const char*		message() const;
    int			nrDone() const;
    const char*		nrDoneText() const;
    int			totalNr() const;
    int			nextStep();

    bool		removenulltrcs_;

protected:

    enum State		{ ReadBuf, WriteBuf, ReadWrite };

    Reader*		rdr_;
    SeisTrcWriter&	wrr_;
    SeisTrcBuf&		buf_;
    SeisTrc&		trc_;
    Seis::GeomType	geomtype_;
    State		state_;
    int			nrread_;
    int			nrwritten_;
    bool		crlsorted_;
    Executor*		postproc_;

    int			doWrite(const SeisTrc&);
    int			readIntoBuf();
    int			analyseBuf();
    Executor*		mkPostProc();

    mutable BufferString	errmsg_;
    mutable BufferString	hndlmsg_;

};


#endif
