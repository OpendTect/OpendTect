#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessing.h"

#include "multiid.h"
#include "seistrc.h"


class SeisTrcReader;

namespace VolProc
{


mClass(VolProcTest) VolumeReader : public ProcessingStep
{
public:
    static void			initClass();

				VolumeReader(ProcessingChain&);

    void			setStorage(const MultiID&);
    const MultiID&		getStorage() const;

    const char*			type() const;
    bool			needsInput(const HorSampling&) const; 

    bool			setCurrentCalcPos(const BinID&);
    bool			compute(int start, int stop);

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

    const char*			errMsg() const;
    
    static const char*		sKeyType()	{ return "VolumeReader";}
    static const char*		sKeyStorageID()	{ return "Input volume"; }

protected:
    static ProcessingStep*	create(ProcessingChain& pc);

    MultiID			storageid_;
    SeisTrcReader*		reader_;
    BufferString		errmsg_;
    SeisTrc			curtrc_;
    bool			validtrc_;
};


};
