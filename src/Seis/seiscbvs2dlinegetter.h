#ifndef seiscbvs2dlinegetter_h
#define seiscbvs2dlinegetter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2009
 RCS:           $Id$
________________________________________________________________________

-*/

#include "executor.h"
#include "uistring.h"

class CBVSSeisTrcTranslator;
class SeisTrcBuf;
namespace Seis { class SelData; }


class SeisCBVS2DLineGetter : public Executor
{
public:
			SeisCBVS2DLineGetter(const char* fnm,SeisTrcBuf&,
					     int trcsperstep,
					     const Seis::SelData&);
			~SeisCBVS2DLineGetter();


    void		addTrc(SeisTrc*);
    int			nextStep();

    uiString		uiMessage() const	{ return msg_; }
    uiString		uiNrDoneText() const	{ return "Traces read"; }

    od_int64		nrDone() const		{ return curnr_; }
    od_int64		totalNr() const		{ return totnr_; }

    int				curnr_;
    int				totnr_;
    SeisTrcBuf&			tbuf_;
    BufferString		fname_;
    uiString			msg_;
    CBVSSeisTrcTranslator*	tr_;
    Seis::SelData*		seldata_;
    int				trcstep_;
    const int			linenr_;
    const int			trcsperstep_;

};

#endif
