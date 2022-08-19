#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "iopar.h"
#include "uistring.h"

class TaskRunner;
class JobCommunic;

namespace VolProc
{

/*!\brief Sits on top and runs ChainOutput for each Geometry */

mExpClass(VolumeProcessing) Processor
{ mODTextTranslationClass(Processor);
public:

				Processor(const IOPar&);
				~Processor()	{}

    bool			run(od_ostream&,JobCommunic* comm=0);
    bool			run(TaskRunner*);

protected:

    IOPar			procpars_;
};

} // namespace VolProc
