#ifndef wvltfact_h
#define wvltfact_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		24-3-1996
 RCS:		$Id: wvltfact.h,v 1.2 2001-07-26 09:44:39 windev Exp $
________________________________________________________________________

@$*/

/*@+
Wavelet factory definitions.
@$*/

#include <prog.h>
#include <wavelet.h>

defineTranslatorGroup(Wavelet,"Wavelet");
defineTranslator(dgb,Wavelet,mDGBKey);

#endif
