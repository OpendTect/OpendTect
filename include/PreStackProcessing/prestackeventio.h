#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2007
________________________________________________________________________


-*/

#include "prestackprocessingmod.h"
#include "executor.h"
#include "bufstringset.h"
#include "trckeyzsampling.h"
#include "iopar.h"
#include "sets.h"
#include "valseriesevent.h"

class IOObj;
class BinIDValueSet;
class TrcKeySampling;
template <class T> class DataInterpreter;

namespace PreStack
{

class EventManager;
class EventPatchReader;
class EventPatchWriter;

/*!
\brief Reader for PreStack events.
*/

mExpClass(PreStackProcessing) EventReader : public Executor
{ mODTextTranslationClass(EventReader);
public:
    			EventReader(IOObj*,EventManager*,bool trigger);
			//!<If not mgr is given, only prepareWork &
			//!<getPositions can be run, no real work
    			~EventReader();

    void		setSelection(const BinIDValueSet*);
    void		setSelection(const TrcKeySampling*);

    bool		prepareWork();
    			//!<Will run automaticly at first nextStep
    bool		getPositions(BinIDValueSet&) const;
    			//!<Only after first nextStep, or prepareWork
			
    bool		getBoundingBox(Interval<int>& inlrg,
	    			       Interval<int>& crlrg ) const;
    			//!<Only after first nextStep, or prepareWork
    static bool		readSamplingData(const IOObj&,SamplingData<int>& inl,
	    				SamplingData<int>& crl);
			
    uiString		uiMessage() const	{ return tr("Loading events"); }
    uiString		errMsg() const;

    static int		encodeEventType(VSEvent::Type);
    static VSEvent::Type decodeEventType(int);

    static const char*	sFileType() 		{ return "Prestack events"; }
    static const char*	sKeyInt16DataChar() 	{ return "Short dc"; }
    static const char*	sKeyInt32DataChar() 	{ return "Int dc"; }
    static const char*	sKeyFloatDataChar() 	{ return "Float dc"; }
    static const char*	sKeyPrimaryDipSource()	{ return "Primary dip"; }
    static const char*	sKeySecondaryDipSource(){ return "Secondary dip"; }
    static const char*	sKeyISamp() 		{ return "In-line sampling"; }
    static const char*	sKeyCSamp() 		{ return "Cross-line sampling";}

    static const char*	sKeyNrHorizons()	{ return "Nr Horizons"; }
    static const char*	sKeyHorizonID()		{ return "Horizon ID"; }
    static const char*	sKeyNextHorizonID()	{ return "Next Horizon ID"; }
    static const char*	sKeyHorizonRef()	{return "Horizon EM Reference";}
    static const char*	sOldHorizonFileName()	{ return "horizoninfo"; }
    static const char*	sAuxDataFileName()	{ return "auxdata"; }
    static const char*	sHorizonFileType()	{ return "PS Horizon info"; }
    static const char*	sHorizonFileVersion()	{ return "PS Horizon version"; }

    int						nextStep();

protected:
    bool					addReader( const char* fnm );
    bool					readAuxData(const char* fnm);

    const IOObj*				ioobj_;
    EventManager*				eventmanager_;
    const BinIDValueSet*			bidsel_;
    const TrcKeySampling*				horsel_;

    ObjectSet<EventPatchReader>			patchreaders_;	

    uiString					errmsg_;
    bool					trigger_;
};


/*!
\brief Writer for PreStack events.
*/

mExpClass(PreStackProcessing) EventWriter : public Executor
{ mODTextTranslationClass(EventWriter);
public:
    			EventWriter(IOObj*,EventManager&);
    			~EventWriter();

    int			nextStep();
    uiString		errMsg() const;
    uiString		uiMessage() const	{ return tr("Storing events"); }


protected:

    bool			writeAuxData(const char* fnm);

    ObjectSet<EventPatchWriter>	patchwriters_;	
    IOObj*			ioobj_;
    IOPar			auxinfo_;
    EventManager&		eventmanager_;
    uiString			errmsg_;
};


/*!
\brief Duplicator for PreStack events.
*/

mExpClass(PreStackProcessing) EventDuplicator : public Executor
{ mODTextTranslationClass(EventDuplicator);
public:
    			EventDuplicator(IOObj* from,IOObj* to);
    			~EventDuplicator();

    od_int64		totalNr() const { return totalnr_; }
    od_int64		nrDone() const { return totalnr_ - filestocopy_.size();}
    uiString		uiMessage() const { return message_; }
    uiString		uiNrDoneText() const { return tr("Files copied"); }

    int			nextStep();
    uiString		errMsg() const { return errmsg_; }

protected:
    void			errorCleanup();

    int				totalnr_;
    BufferStringSet		filestocopy_;
    uiString			errmsg_;
    uiString			message_;

    IOObj*			from_;
    IOObj*			to_;
};


}; //namespace

