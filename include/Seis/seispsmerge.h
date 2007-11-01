#ifndef seispsmerge_h
#define seispsmerge_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	R. K. Singh
 Date:		Oct 2007
 RCS:		$Id: seispsmerge.h,v 1.1 2007-11-01 07:08:04 cvsraman Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "cubesampling.h"
class IOObj;
class IOPar;
class SeisTrc;
class MultiID;
class SeisCBVSPSReader;
class SeisCBVSPSWriter;
class SeisPacketInfo;


/*!\brief Single trace processing executor

When a trace info is read, the selection callback is triggered. You can then
use skipCurTrc(). When the trace is read, the processing callback is triggered.
You can set you own trace as output trace, otherwise the input trace will be
taken.

*/

class SeisPSMerger : public Executor
{
public:
			SeisPSMerger(ObjectSet<IOObj>,const IOObj*);
    virtual		~SeisPSMerger();

    virtual const char*	message() const;
    virtual const char*	nrDoneText() const;
    virtual int		nrDone() const;
    virtual int		totalNr() const;
    virtual int		nextStep();

protected:

    ObjectSet<IOObj>	inobjs_;
    const IOObj*	outobj_;
    int			curinlidx_;
    BinID		curbid_;
    int			nrobjs_;

    TypeSet<int>		inlset_;
    TypeSet<TypeSet<int> >	lineobjlist_;
    ObjectSet<SeisPacketInfo>	pinfoset_;

    ObjectSet<SeisCBVSPSReader>		readers_;
    SeisCBVSPSWriter*      		writer_;

    BufferString	msg_;
    int			totnr_;
    int			nrdone_;

    bool		init();
    int			prepareReaders();
    int			doNextPos();
};


#endif
