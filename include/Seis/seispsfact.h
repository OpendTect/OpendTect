#ifndef seispsfact_h
#define seispsfact_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		21-3-1996
 RCS:		$Id: seispsfact.h,v 1.2 2008-01-08 11:54:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "seispsioprov.h"

#define sKeySeisPSTranslatorGroup "Pre-Stack Seismics"
#define sKeySeisPS2DTranslatorGroup "Pre-Stack Seismics (2D)"
defineTranslatorGroup(SeisPS,sKeySeisPSTranslatorGroup);
defineTranslator(CBVS,SeisPS,"CBVS");
defineTranslatorGroup(SeisPS2D,sKeySeisPS2DTranslatorGroup);
defineTranslator(CBVS,SeisPS2D,"CBVS");


#endif
