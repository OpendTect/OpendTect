#ifndef prestackeventio_h
#define prestackeventio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		March 2007
 RCS:		$Id: prestackeventio.h,v 1.4 2009-01-06 06:05:40 cvsranojay Exp $
________________________________________________________________________


-*/

#include "executor.h"
#include "cubesampling.h"
#include "sets.h"
#include "bufstringset.h"
#include "valseriesevent.h"

class IOObj;
class Conn;
class BinIDValueSet;
class BinID;
class HorSampling;
template <class T> class DataInterpreter;

namespace PreStack
{

class EventManager;
class EventPatchReader;

/*! Reader for prestack events. */

mClass EventReader : public Executor
{
public:
    			EventReader(IOObj*,EventManager*);
			//!<If not mgr is given, only prepareWork &
			//!<getPositions can be run, no real work
    			~EventReader();

    void		setSelection(const BinIDValueSet*);
    void		setSelection(const HorSampling*);

    bool		prepareWork();
    			//!<Will run automaticly at first nextStep
    bool		getPositions(BinIDValueSet&) const;
    			//!<Only after first nextStep, or prepareWork
			
    bool		getBoundingBox(Interval<int>& inlrg,
	    			       Interval<int>& crlrg ) const;
    			//!<Only after first nextStep, or prepareWork

    static int		encodeEventType(VSEvent::Type);
    static VSEvent::Type decodeEventType(int);

    static const char*	sFileType() 		{ return "Prestack events"; }
    static const char*	sKeyInt16DataChar() 	{ return "Short dc"; }
    static const char*	sKeyInt32DataChar() 	{ return "Int dc"; }
    static const char*	sKeyFloatDataChar() 	{ return "Float dc"; }
    static const char*	sKeyPrimaryDipSource()	{ return "Primary dip"; }
    static const char*	sKeySecondaryDipSource(){ return "Secondary dip"; }

    static const char*	sKeyNrHorizons()	{ return "Nr Horizons"; }
    static const char*	sKeyHorizonID()		{ return "Horizon ID"; }
    static const char*	sKeyNextHorizonID()	{ return "Next Horizon ID"; }
    static const char*	sKeyHorizonRef()	{ return "Horizon EM Reference"; }
    static const char*	sHorizonFileName()	{ return "horizoninfo"; }
    static const char*	sHorizonFileType()	{ return "PS Horizon info"; }
    static const char*	sHorizonFileVersion()	{ return "PS Horizon version"; }

    int						nextStep();

protected:
    void					addReader( const char* fnm );
    bool					readHorizonIDs(const char* fnm);

    const IOObj*				ioobj_;
    EventManager*				eventmanager_;
    const BinIDValueSet*			bidsel_;
    const HorSampling*				horsel_;

    ObjectSet<SequentialTask>			patchreaders_;	
};


mClass EventWriter : public Executor
{
public:
    			EventWriter(IOObj*,EventManager&);
    			~EventWriter();

    int			nextStep();
    const char*		errMsg() const;

    static const char*	sKeyISamp() { return "In-line sampling"; }
    static const char*	sKeyCSamp() { return "Cross-line sampling"; }

protected:

    bool			writeHorizonIDs( const char* fnm ) const;

    ObjectSet<SequentialTask>	patchwriters_;	
    IOObj*			ioobj_;
    EventManager&		eventmanager_;
    BufferString		errmsg_;
};


mClass EventDuplicator : public Executor
{
public:
    			EventDuplicator(IOObj* from,IOObj* to);
    			~EventDuplicator();

    od_int64		totalNr() const { return totalnr_; }
    od_int64		nrDone() const { return totalnr_ - filestocopy_.size();}
    const char*		message() const { return message_.buf(); }
    const char*		nrDoneText() const { return "Files copied"; }

    int			nextStep();
    const char*		errMsg() const { return errmsg_[0] ? errmsg_.buf() : 0;}

protected:
    void		errorCleanup();

    int				totalnr_;
    BufferStringSet		filestocopy_;
    BufferString		errmsg_;
    BufferString		message_;

    IOObj*			from_;
    IOObj*			to_;
};


}; //namespace

#endif
