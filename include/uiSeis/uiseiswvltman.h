#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseiswvltman.h,v 1.10 2009-03-19 09:01:55 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
#include "datapack.h"

class uiFlatViewer;
template <class T> class Array2D;


mClass uiSeisWvltMan : public uiObjFileMan
{
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

protected:

    uiFlatViewer*	wvltfld;
    DataPack::ID	curid_;

    void		mkFileInfo();

    void		impPush(CallBacker*);
    void		crPush(CallBacker*);
    void		getFromOtherSurvey(CallBacker*);

};


#endif
