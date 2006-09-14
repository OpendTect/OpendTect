#ifndef seiseventsnapper_h
#define seiseventsnapper_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          September 2006
 RCS:           $Id: seiseventsnapper.h,v 1.1 2006-09-14 20:10:39 cvsnanne Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "valseriesevent.h"

class BinIDValueSet;
class IOObj;
class SeisRequester;

class SeisEventSnapper : public Executor
{
public:
				SeisEventSnapper(const IOObj&,BinIDValueSet&);
				~SeisEventSnapper();

    void			setEvent( VSEvent::Type tp )
    				{ eventtype_ = tp; }
    VSEvent::Type		getEvent() const	{ return eventtype_; }

    void			setSearchGate( const Interval<float>& gate )
				{ searchgate_ = gate; }
    const Interval<float>&	getSearchGate() const	{ return searchgate_; }

    virtual int			totalNr() const		{ return totalnr_; }
    virtual int			nrDone() const		{ return nrdone_; }

protected:

    virtual int			nextStep();

    BinIDValueSet&		positions_;
    SeisRequester*		req_;
    Interval<float>		searchgate_;
    VSEvent::Type		eventtype_;

    int				totalnr_;
    int				nrdone_;

};

#endif
