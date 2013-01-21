#ifndef prestackmute_h
#define prestackmute_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "prestackprocessor.h"
#include "multiid.h"

class Muter;

namespace PreStack
{
class MuteDef;

mExpClass(PreStackProcessing) Mute : public Processor
{
public:
			mDefaultFactoryInstantiation( Processor, Mute,
				"Mute", sFactoryKeyword() );

 			Mute();
    			~Mute();

    bool		prepareWork();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    const char*		errMsg() const		{ return errmsg_.str(); }

    static const char*	sTaperLength()		{ return "Taper Length";}
    static const char*	sTailMute()		{ return "Tail Mute";}
    static const char*	sMuteDef()		{ return "Mute Definition";}

    const MultiID&	muteDefID() const	{ return id_; }
    const MuteDef&	muteDef() const		{ return def_; }
    MuteDef&		muteDef()		{ return def_; }
    bool		isTailMute() const	{ return tail_; }
    float		taperLength() const	{ return taperlen_; }
    bool		setMuteDefID(const MultiID&);
    void		setEmptyMute();
    void		setTailMute(bool yn=true);
    void		setTaperLength(float l);

protected:

    MuteDef&			def_;
    Muter*			muter_;
    MultiID			id_;
    BufferString		errmsg_;

    od_int64			nrIterations() const { return outidx_.size(); }
    bool			doWork(od_int64,od_int64,int);

    bool			tail_;
    float			taperlen_;

    TypeSet<int>		outidx_;
    TypeSet<int>		offsets_;
};


}; //namespace

#endif

