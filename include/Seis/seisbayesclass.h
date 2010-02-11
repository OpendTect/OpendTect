#ifndef seisbayesclass_h
#define seisbayesclass_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2010
 RCS:		$Id: seisbayesclass.h,v 1.1 2010-02-11 16:10:44 cvsbert Exp $
________________________________________________________________________

*/

#include "executor.h"
class IOPar;
class ProbDenFunc;
class SeisTrcReader;
class SeisTrcWriter;


mClass SeisBayesClass : public Executor
{
public:

    				SeisBayesClass(const IOPar&);
    				~SeisBayesClass();

    static const char*		sKeyPDFID();
    static const char*		sKeySeisInpID();
    static const char*		sKeySeisOutID();

    int				nextStep();
    const char*			message() const;
    const char*			nrDoneText() const;
    od_int64			nrDone() const;
    od_int64			totalNr() const;

protected:

    bool			is2d_;
    ObjectSet<ProbDenFunc>	pdfs_;
    ObjectSet<SeisTrcReader>	rdrs_;
    ObjectSet<SeisTrcWriter>	wrrs_;
    bool			donorm_;

    od_int64			nrdone_;
    od_int64			totalnr_;
    BufferString		msg_;

};


#endif
