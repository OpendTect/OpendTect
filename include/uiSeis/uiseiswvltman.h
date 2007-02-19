#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseiswvltman.h,v 1.5 2007-02-19 16:41:45 cvsbert Exp $
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
    Array2D<float>*	fda2d_;

    void		mkFileInfo();

    void		impPush(CallBacker*);
    void		crPush(CallBacker*);
};


#endif
