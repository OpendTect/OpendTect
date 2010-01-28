/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Jul 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: probdenfunctr.cc,v 1.1 2010-01-28 09:46:38 cvsnanne Exp $";

#include "probdenfunctr.h"

defineTranslatorGroup(ProbDenFunc,"Probability Density Function");
defineTranslator(dgb,ProbDenFunc,mDGBKey);

mDefSimpleTranslatorSelector(ProbDenFunc,sKeyProbDenFunc())
mDefSimpleTranslatorioContext(ProbDenFunc,Feat)

bool dgbProbDenFuncTranslator::read( ArrayNDProbDenFunc& pdf,
				     const IOObj& ioobj )
{
    return true;
}


bool dgbProbDenFuncTranslator::write( const ArrayNDProbDenFunc& pdf,
				      const IOObj& ioobj )
{
    return true;
}
