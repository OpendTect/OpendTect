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
#include "seistype.h"
#include "filepath.h"

namespace Seis { class SelData; class Storer; }

namespace SEGY
{

namespace Vintage
{

class Info;

mExpClass(uiSEGYTools) Importer : public ExecutorGroup
{ mODTextTranslationClass(SEGY::Vintage::Importer)
public:

		Importer(const Info&,const OD::String& trnalnm,
			 const Seis::GeomType,Seis::SelData*,
			 const char* attr2dnm=0);
		~Importer();

protected:

    ObjectSet<Seis::Storer>	seistrcstorers_;

};


mExpClass(uiSEGYTools) Info
{ mODTextTranslationClass(SEGY::Vintage::Info)
public:

    BufferString	vintagenm_;
    BufferStringSet	filenms_;
    File::Path		fp_;

};

} //namespace Vintage

} //namespace SEGY
