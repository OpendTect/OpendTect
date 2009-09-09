#ifndef uiseiswvltman_h
#define uiseiswvltman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseiswvltman.h,v 1.15 2009-09-09 07:46:05 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
#include "datapack.h"

class uiFlatViewer;
class uiWaveletExtraction;
class Wavelet;
template <class T> class Array2D;


mClass uiSeisWvltMan : public uiObjFileMan
{
public:
			uiSeisWvltMan(uiParent*);
			~uiSeisWvltMan();

protected:

    uiFlatViewer*		wvltfld;
    DataPack::ID		curid_;
    uiWaveletExtraction*  	wvltext_;

    void			mkFileInfo();
    void			setViewerData(const Wavelet*);
    
    void			impPush(CallBacker*);
    void			crPush(CallBacker*);
    void			extractPush(CallBacker*);
    void			getFromOtherSurvey(CallBacker*);
    void			reversePolarity(CallBacker*);
    void			rotatePhase(CallBacker*);
    void                	updateCB(CallBacker*);
    void                	closeDlg(CallBacker*);
    void 			updateViewer(CallBacker*);

};


#endif
