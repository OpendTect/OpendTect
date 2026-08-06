#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "bufstringset.h"
#include "executor.h"
#include "iopar.h"
#include "sets.h"
#include "trckeyzsampling.h"
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
{
mODTextTranslationClass(EventReader)
public:
			EventReader(const IOObj&,EventManager*,bool trigger);
			//!<If not mgr is given, only prepareWork &
			//!<getPositions can be run, no real work
			~EventReader();
			mOD_DisableCopy(EventReader)

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

    uiString		uiMessage() const override
			{ return tr("Loading events"); }
    od_int64		totalNr() const override	{ return totalnr_; }
    od_int64		nrDone() const override		{ return nrdone_; }

    uiString		errMsg() const;

    static int		encodeEventType(VSEvent::Type);
    static VSEvent::Type decodeEventType(int);

    static const char*	sFileType()		{ return "Prestack events"; }
    static const char*	sKeyInt16DataChar()	{ return "Short dc"; }
    static const char*	sKeyInt32DataChar()	{ return "Int dc"; }
    static const char*	sKeyFloatDataChar()	{ return "Float dc"; }
    static const char*	sKeyPrimaryDipSource()	{ return "Primary dip"; }
    static const char*	sKeySecondaryDipSource(){ return "Secondary dip"; }
    static const char*	sKeyISamp()		{ return "In-line sampling"; }
    static const char*	sKeyCSamp()		{ return "Cross-line sampling";}

    static const char*	sKeyNrHorizons()	{ return "Nr Horizons"; }
    static const char*	sKeyHorizonID()		{ return "Horizon ID"; }
    static const char*	sKeyNextHorizonID()	{ return "Next Horizon ID"; }
    static const char*	sKeyHorizonRef()	{return "Horizon EM Reference";}
    static const char*	sOldHorizonFileName()	{ return "horizoninfo"; }
    static const char*	sAuxDataFileName()	{ return "auxdata"; }
    static const char*	sHorizonFileType()	{ return "PS Horizon info"; }
    static const char*	sHorizonFileVersion()	{ return "PS Horizon version"; }

protected:
    bool		doPrepare(od_ostream*) override;
    int			nextStep() override;

    bool		addReader(const char* fnm);
    bool		readAuxData(const char* fnm);

    const IOObj*			ioobj_;
    EventManager*			eventmanager_;
    const BinIDValueSet*		bidsel_			= nullptr;
    const TrcKeySampling*		horsel_			= nullptr;

    ManagedObjectSet<EventPatchReader>	patchreaders_;

    uiString				errmsg_;
    bool				trigger_;

    od_int64				totalnr_		= 0;
    od_int64				nrdone_			= 0;
};


/*!
\brief Writer for PreStack events.
*/

mExpClass(PreStackProcessing) EventWriter : public Executor
{
mODTextTranslationClass(EventWriter)
public:
			EventWriter(const IOObj&,EventManager&);
			~EventWriter();
			mOD_DisableCopy(EventWriter)

    uiString		errMsg() const;
    uiString		uiMessage() const override
			{ return tr("Storing events"); }
    od_int64		totalNr() const override	{ return totalnr_; }
    od_int64		nrDone() const override		{ return nrdone_; }

protected:

    bool		doPrepare(od_ostream*) override;
    bool		writeAuxData(const char* fnm);
    int			nextStep() override;

    ManagedObjectSet<EventPatchWriter>	patchwriters_;
    const IOObj*			ioobj_;
    IOPar				auxinfo_;
    EventManager&			eventmanager_;
    uiString				errmsg_;

    od_int64				totalnr_		= 0;
    od_int64				nrdone_			= 0;
};


/*!
\brief Duplicator for PreStack events.
*/

mExpClass(PreStackProcessing) EventDuplicator : public Executor
{
mODTextTranslationClass(EventDuplicator)
public:
			EventDuplicator(const IOObj& from,const IOObj& to);
			~EventDuplicator();
			mOD_DisableCopy(EventDuplicator)

    od_int64		totalNr() const override { return totalnr_; }
    od_int64		nrDone() const override
			{ return totalnr_ - filestocopy_.size();}

    uiString		uiMessage() const override { return message_; }
    uiString		uiNrDoneText() const override
			{ return tr("Files copied"); }

    int			nextStep() override;
    uiString		errMsg() const { return errmsg_; }

protected:
    void			errorCleanup();

    od_int64			totalnr_			= -1;
    BufferStringSet		filestocopy_;
    uiString			errmsg_;
    uiString			message_;

    const IOObj*		from_;
    const IOObj*		to_;
};

} // namespace PreStack
