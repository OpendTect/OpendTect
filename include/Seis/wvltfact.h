#ifndef wvltfact_h
#define wvltfact_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: wvltfact.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

@$*/

/*@+
Wavelet factory definitions.
@$*/

#include <wavelet.h>

defineTranslatorGroup(Wavelet,"Wavelet");
defineTranslator(dgb,Wavelet,mDGBKey);

#endif
