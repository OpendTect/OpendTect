#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseiswvltman.h,v 1.13 2009-07-22 16:01:23 cvsbert Exp $
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
    void		extractPush(CallBacker*);
    void		getFromOtherSurvey(CallBacker*);
    void		reversePolarity(CallBacker*);

};


#endif
