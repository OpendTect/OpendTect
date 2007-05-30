#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseiswvltman.h,v 1.7 2007-05-30 13:26:55 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiFlatViewer;
template <class T> class Array2D;


class uiSeisWvltMan : public uiObjFileMan
{
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

protected:

    uiFlatViewer*	wvltfld;
    Array2D<float>*	fva2d_;

    void		mkFileInfo();

    void		impPush(CallBacker*);
    void		crPush(CallBacker*);
    void		getFromOtherSurvey(CallBacker*);
};


#endif
