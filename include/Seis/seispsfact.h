#ifndef seispsfact_h
#define seispsfact_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		2005
 RCS:		$Id: seispsfact.h,v 1.6 2008-11-20 13:24:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "seispsioprov.h"
#include "seismulticubeps.h"
#include "segydirecttr.h"

#define sKeySeisPS3DTranslatorGroup "Pre-Stack Seismics"
defineTranslatorGroup(SeisPS3D,sKeySeisPS3DTranslatorGroup);
defineTranslator(CBVS,SeisPS3D,"CBVS");
defineTranslator(MultiCube,SeisPS3D,"MultiCube");
defineTranslator(SEGYDirect,SeisPS3D,"SEGYDirect");

#define sKeySeisPS2DTranslatorGroup "2D Pre-Stack Seismics"
defineTranslatorGroup(SeisPS2D,sKeySeisPS2DTranslatorGroup);
defineTranslator(CBVS,SeisPS2D,"CBVS");
defineTranslator(SEGYDirect,SeisPS2D,"SEGYDirect");


#endif
