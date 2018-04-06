#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	nageswara

 RCS:		$Id: $
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "executor.h"

#include "seisselection.h"
#include "filepath.h"

class SeisTrcWriter;
namespace SEGY { namespace Vintage {class Info; } }

namespace SEGY
{

namespace Vintage
{

mExpClass(uiSEGY) Importer : public ExecutorGroup
{ mODTextTranslationClass(Importer)
public:
		Importer(const Info&, const OD::String& trnalnm,
			 const Seis::GeomType gt, Seis::SelData* sd);
		~Importer();
protected:
    ObjectSet<SeisTrcWriter>	seistrcwriters_;
};


mExpClass(uiSEGY) Info
{ mODTextTranslationClass(Info)
public:
    BufferString	vintagenm_;
    BufferStringSet	filenms_;
    File::Path		fp_;
};

} //namespace Vintage

} //namespace SEGY
