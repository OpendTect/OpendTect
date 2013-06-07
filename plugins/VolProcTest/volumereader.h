#ifndef volumereader_h
#define volumereader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		3-28-2007
 RCS:		$Id: volumereader.h,v 1.2 2009/07/22 16:01:27 cvsbert Exp $
________________________________________________________________________
    
-*/


#include "volumeprocessing.h"

#include "multiid.h"
#include "seistrc.h"


class SeisTrcReader;

namespace VolProc
{


class VolumeReader : public ProcessingStep
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
#endif
