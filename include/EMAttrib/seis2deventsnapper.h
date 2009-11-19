#ifndef seis2deventsnapper_h
#define seis2deventsnapper_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          October 2009
 RCS:           $Id: seis2deventsnapper.h,v 1.1 2009-11-19 03:48:34 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "seiseventsnapper.h"
#include "seistrc.h"
#include "seisread.h"
#include "executor.h"
#include "emposid.h"

namespace EM { class Hor2DSeisLineIterator; class Horizon2D; }
namespace Seis { class Horizon2D; }
class SeisTrcReader;
class IOObj;


mClass Seis2DEventSnapper : public SeisEventSnapper
{
public:
				Seis2DEventSnapper(EM::Horizon2D&,const IOObj*,
						   const LineKey&,
						   const Interval<float>& gate);
				~Seis2DEventSnapper();

protected:
    virtual int			nextStep();

    int				horlineidx_;
    EM::PosID			posid_;
    SeisTrc			trc_;
    SeisTrcReader*		seisrdr_;
    EM::Horizon2D&		hor_;
};


mClass Seis2DLineSetEventSnapper : public ExecutorGroup
{
public:
				Seis2DLineSetEventSnapper(EM::Horizon2D*,
					  const BufferString& attrnm,int type,
					  const Interval<float>& gate);
				~Seis2DLineSetEventSnapper();
protected:
    int				type_;
    BufferString		attribnm_;
    Interval<float>		gate_;
    EM::Horizon2D*		hor_;
    EM::Hor2DSeisLineIterator*	hor2diterator_;
};

#endif
