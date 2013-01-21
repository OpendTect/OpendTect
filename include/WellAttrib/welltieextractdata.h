#ifndef welltieextractdata_h
#define welltieextractdata_h

/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2009
RCS:           $Id: welltieextractdata.h,v 1.1 2009-01-19 13:02:33 cvsbruno Exp
$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "executor.h"
#include "position.h"

class CubeSampling;
class LineKey;
class MultiID;
class IOObj;
class SeisTrcReader;
class SeisTrcBuf;
class SeisTrc;

namespace WellTie
{

mExpClass(WellAttrib) SeismicExtractor : public Executor
{
public:
			SeismicExtractor(const IOObj&);
			~SeismicExtractor();

    int                 nextStep();
    od_int64            totalNr() const		{ return extrintv_.nrSteps(); }
    od_int64            nrDone() const          { return nrdone_; }
    const char*         message() const         { return "Computing..."; }
    const char*         nrDoneText() const      { return "Points done"; }
    void		setBIDValues(const TypeSet<BinID>&); 
    void		setInterval(const StepInterval<float>&);
    //Only 2D
    void		setLineKey(const LineKey* lk) { linekey_ = lk; }
    void		setAttrNm(const char* nm) { attrnm_ = nm; }

    const SeisTrc&	result() const		{ return *outtrc_; }
    const char* 	errMsg() const	
    			{ return errmsg_.isEmpty() ? 0 : errmsg_.buf(); }
    
protected:

    const char*		attrnm_;
    int                 nrdone_;
    int			radius_;
    CubeSampling* 	cs_;
    TypeSet<BinID>	bidset_;
    SeisTrc*		outtrc_;
    SeisTrcBuf*		trcbuf_;
    SeisTrcReader* 	rdr_;
    StepInterval<float> extrintv_;
    const LineKey*	linekey_;
    BufferString	errmsg_;
  
    bool		collectTracesAroundPath();
};

};//namespace WellTie

#endif

