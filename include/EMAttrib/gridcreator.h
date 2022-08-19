#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "executor.h"

#include "bufstringset.h"

class TrcKeyZSampling;
class IOObj;
class TaskRunner;
namespace EM { class Horizon2D; }

/*!
\brief ExecutorGroup to create 2D seismic grid from 3D.
*/

mExpClass(EMAttrib) Seis2DGridCreator : public ExecutorGroup
{ mODTextTranslationClass(Seis2DGridCreator);
public:
    			Seis2DGridCreator(const IOPar&);
			~Seis2DGridCreator();

    uiString		uiNrDoneText() const override;

    static const char*	sKeyOverWrite();
    static const char*	sKeyInput();
    static const char*	sKeyOutput();
    static const char*  sKeySelType();
    static const char*	sKeyOutpAttrib();
    static const char*	sKeyInlSelType();
    static const char*	sKeyCrlSelType();

    static const char*	sKeyBaseLine();
    static const char*	sKeyStartBinID();
    static const char*	sKeyStopBinID();
    static const char*	sKeyInlSpacing();
    static const char*	sKeyCrlSpacing();

    static const char*	sKeyInlPrefix();
    static const char*	sKeyCrlPrefix();
    bool		hasWarning(BufferString&) const;


protected:

    BufferStringSet	failedlines_;
    bool		init(const IOPar&);
    bool		initFromInlCrl(const IOPar&,const IOObj&,const IOObj&,
				       const TrcKeyZSampling&);
    bool		initFromRandomLine(const IOPar&,const IOObj&,
					   const IOObj&,const TrcKeyZSampling&);
    Pos::GeomID		getGeomID(const char* lnm);
};


/*!
\brief ExecutorGroup to create 2D horizon grid from 3D.
*/

mExpClass(EMAttrib) Horizon2DGridCreator : public ExecutorGroup
{ mODTextTranslationClass(Horizon2DGridCreator);
public:
    			Horizon2DGridCreator();
			~Horizon2DGridCreator();

    od_int64		totalNr() const override;
    od_int64		nrDone() const override;
    uiString		uiNrDoneText() const override;

    bool		init(const IOPar&,TaskRunner* tr=0);
    bool		finish(TaskRunner* tr=0);

    static const char*	sKeyInputIDs();
    static const char*	sKeySeisID();
    static const char*	sKeyPrefix();

protected:

    ObjectSet<EM::Horizon2D> horizons_;
    od_int64		nrdone_;
    od_int64		totalnr_;

};
