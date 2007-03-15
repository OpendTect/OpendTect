#ifndef prestackmute_h
#define prestackmute_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Nov 2006
 RCS:		$Id: prestackmute.h,v 1.1 2007-03-15 17:28:52 cvskris Exp $
________________________________________________________________________


-*/

#include "prestackgatherprocessor.h"
#include "multiid.h"

class Muter;

namespace PreStack
{
class MuteDef;

class Mute : public Processor
{
public:

    static void		initClass();
    static Processor*	createFunc();

 			Mute();
    			~Mute();

    bool		prepareWork();

    const char*		name() const		{ return sName(); }
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    const char*		errMsg() const		{ return errmsg_; }

    static const char*	sName()			{ return "Mute"; }
    static const char*	sTaperLength()		{ return "Taper Length";}
    static const char*	sTailMute()		{ return "Tail Mute";}
    static const char*	sMuteDef()		{ return "Mute Definition";}

    const MultiID&	muteDefID() const	{ return id_; }
    bool		isTailMute() const	{ return tail_; }
    float		taperLength() const	{ return taperlen_; }
    void		setMuteDefID( const MultiID& id ) { id_ = id; }
    void		setTailMute( bool yn=true )	  { tail_ = yn; }
    void		setTaperLength( float l )	  { taperlen_ = l; }

protected:

    MuteDef&			def_;
    Muter*			muter_;
    MultiID			id_;
    BufferString		errmsg_;

    bool			doWork(int,int,int);

    bool			tail_;
    float			taperlen_;
};


}; //namespace

#endif
