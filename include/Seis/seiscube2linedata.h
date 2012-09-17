#ifndef seiscube2linedata_h
#define seiscube2linedata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2010
 RCS:		$Id: seiscube2linedata.h,v 1.2 2010/04/26 08:29:58 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "executor.h"
#include "bufstringset.h"
class IOObj;
class SeisTrcBuf;
class Seis2DLineSet;
class SeisTrcReader;
class SeisTrcWriter;
class Cube2LineDataLineKeyProvider;


/*!\brief Extracts 3D cube data into 2D line attribute */

mClass SeisCube2LineDataExtracter : public Executor
{
public:
		SeisCube2LineDataExtracter(const IOObj& cubein,
					   const IOObj& lsout,
					   const char* attrnm,
					   const BufferStringSet* lnms=0);
		~SeisCube2LineDataExtracter();

    const char*	message() const		{ return msg_; }
    const char*	nrDoneText() const	{ return "Traces written"; }
    od_int64	nrDone() const		{ return nrdone_; }
    od_int64	totalNr() const		{ return -1; }

    int		nextStep();

protected:

    SeisTrcReader&	rdr_;
    Seis2DLineSet&	ls_;
    SeisTrcWriter&	wrr_;
    SeisTrcBuf&		tbuf_;
    const BufferString	attrnm_;
    BufferString	msg_;
    BufferStringSet	lnms_;
    Executor*		fetcher_;
    int			lidx_;
    od_int64		nrdone_;
    Cube2LineDataLineKeyProvider* c2ldlkp_;
    BufferStringSet	lineshandled_;

    void		closeDown();
    bool		getNextFetcher();
    int			handleTrace();

    friend class	Cube2LineDataLineKeyProvider;

};


#endif
