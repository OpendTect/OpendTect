#ifndef seiscbvs2dlinegetter_h
#define seiscbvs2dlinegetter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2009
 RCS:           $Id: seiscbvs2dlinegetter.h,v 1.1 2009/07/22 16:00:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
class SeisTrcBuf;
class CBVSSeisTrcTranslator;
namespace Seis { class SelData; }


mClass SeisCBVS2DLineGetter : public Executor
{
public:

			SeisCBVS2DLineGetter(const char*,SeisTrcBuf&,int,
					     const Seis::SelData&);
			~SeisCBVS2DLineGetter();


    void		addTrc(SeisTrc*);
    int			nextStep();

    const char*		message() const		{ return msg; }
    const char*		nrDoneText() const	{ return "Traces read"; }
    od_int64		nrDone() const		{ return curnr; }
    od_int64		totalNr() const		{ return totnr; }

    int			curnr;
    int			totnr;
    SeisTrcBuf&		tbuf;
    BufferString	fname;
    BufferString	msg;
    CBVSSeisTrcTranslator* tr;
    Seis::SelData*	seldata;
    int			trcstep;
    const int		linenr;
    const int		trcsperstep;

};


#endif
