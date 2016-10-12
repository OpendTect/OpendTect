#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		Feb 2016
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
