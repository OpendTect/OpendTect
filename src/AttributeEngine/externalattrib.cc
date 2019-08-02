/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2006
________________________________________________________________________

-*/

#include "externalattrib.h"

#include "attribdesc.h"
#include "attribsel.h"


namespace Attrib
{

mImplClassFactory( ExtAttribCalc, factory );


RefMan<RegularSeisDataPack> ExtAttribCalc::createAttrib( const TrcKeyZSampling&,
				DataPack::ID, TaskRunner* )
{ return 0; }


bool ExtAttribCalc::createAttrib( ObjectSet<BinnedValueSet>&, TaskRunner* )
{ return false; }


bool ExtAttribCalc::createAttrib( const BinnedValueSet&, SeisTrcBuf& buf,
				  TaskRunner* )
{ return false; }

} // namespace Attrib
