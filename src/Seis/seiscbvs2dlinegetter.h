#ifndef seiscbvs2dlinegetter_h
#define seiscbvs2dlinegetter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2009
 RCS:           $Id: seiscbvs2dlinegetter.h,v 1.2 2012-08-03 13:01:35 cvskris Exp $
________________________________________________________________________

-*/

#include "executor.h"
class SeisTrcBuf;
class CBVSSeisTrcTranslator;
namespace Seis { class SelData; }


class SeisCBVS2DLineGetter : public Executor
{
public:

			SeisCBVS2DLineGetter(const char*,SeisTrcBuf&,int,
					     const Seis::SelData&);
			~SeisCBVS2DLineGetter();


    void		addTrc(SeisTrc*);
    int			nextStep();

    const char*		message() const		{ return msg_; }
    const char*		nrDoneText() const	{ return "Traces read"; }
    od_int64		nrDone() const		{ return curnr_; }
    od_int64		totalNr() const		{ return totnr_; }

    int				curnr_;
    int				totnr_;
    SeisTrcBuf&			tbuf_;
    BufferString		fname_;
    BufferString		msg_;
    CBVSSeisTrcTranslator*	tr_;
    Seis::SelData*		seldata_;
    int				trcstep_;
    const int			linenr_;
    const int			trcsperstep_;

};


#endif
