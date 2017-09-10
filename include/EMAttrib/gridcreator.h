#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "executor.h"
#include "taskrunner.h"

#include "bufstringset.h"

class TrcKeyZSampling;
class IOObj;
namespace EM { class Horizon2D; }

/*!
\brief ExecutorGroup to create 2D seismic grid from 3D.
*/

mExpClass(EMAttrib) Seis2DGridCreator : public ExecutorGroup
{ mODTextTranslationClass(Seis2DGridCreator);
public:
			Seis2DGridCreator(const IOPar&);
			~Seis2DGridCreator();

    virtual uiString	nrDoneText() const;

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
};


/*!
\brief ExecutorGroup to create 2D horizon grid from 3D.
*/

mExpClass(EMAttrib) Horizon2DGridCreator : public ExecutorGroup
{ mODTextTranslationClass(Horizon2DGridCreator);
public:
			Horizon2DGridCreator();
			~Horizon2DGridCreator();

    virtual od_int64	totalNr() const;
    virtual od_int64	nrDone() const;
    virtual uiString	nrDoneText() const;

    bool		init(const IOPar&,const TaskRunnerProvider&);
    bool		finish(const TaskRunnerProvider&);

    static const char*	sKeyInputIDs();
    static const char*	sKeySeisID();
    static const char*	sKeyPrefix();

protected:

    ObjectSet<EM::Horizon2D> horizons_;
    od_int64		nrdone_;
    od_int64		totalnr_;

};
