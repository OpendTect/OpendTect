#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseiswvltman.h,v 1.4 2006-10-20 12:06:29 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiSectionDisp;
template <class T> class Array2D;
namespace FlatDisp { class Data; class Context; }


class uiSeisWvltMan : public uiObjFileMan
{
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

protected:

    uiSectionDisp*	wvltfld;
    Array2D<float>*	fda2d_;
    FlatDisp::Data*	fddata_;
    FlatDisp::Context&	fdctxt_;

    void		mkFileInfo();

    void		impPush(CallBacker*);
    void		crPush(CallBacker*);
};


#endif
