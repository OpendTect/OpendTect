#ifndef seispsfact_h
#define seispsfact_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2005
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seispsioprov.h"
#include "seismulticubeps.h"
#include "segydirecttr.h"

defineTranslatorGroup(SeisPS3D,"Pre-Stack Seismics");
mDefSimpleTranslatorSelector(SeisPS3D);
uiString SeisPS3DTranslatorGroup::sTypeName()
{ return tr("Pre-Stack Seismics"); }

defineTranslator(CBVS,SeisPS3D,"CBVS");
defineTranslator(MultiCube,SeisPS3D,"MultiCube");
defineTranslator(SEGYDirect,SeisPS3D,mSEGYDirectTranslNm);

defineTranslatorGroup(SeisPS2D,"2D Pre-Stack Seismics");
mDefSimpleTranslatorSelector(SeisPS2D);
uiString SeisPS2DTranslatorGroup::sTypeName()
{ return tr("2D Pre-Stack Seismics"); }

defineTranslator(CBVS,SeisPS2D,"CBVS");
defineTranslator(SEGYDirect,SeisPS2D,mSEGYDirectTranslNm);


#endif
