#ifndef seispsfact_h
#define seispsfact_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		2005
 RCS:		$Id: seispsfact.h,v 1.4 2008-01-22 15:04:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "seispsioprov.h"

#define sKeySeisPS3DTranslatorGroup "Pre-Stack Seismics"
defineTranslatorGroup(SeisPS3D,sKeySeisPS3DTranslatorGroup);
defineTranslator(CBVS,SeisPS3D,"CBVS");

#define sKeySeisPS2DTranslatorGroup "2D Pre-Stack Seismics"
defineTranslatorGroup(SeisPS2D,sKeySeisPS2DTranslatorGroup);
defineTranslator(CBVS,SeisPS2D,"CBVS");


#endif
