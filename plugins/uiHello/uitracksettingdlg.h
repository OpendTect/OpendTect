#ifndef uitracksettingdlg_h
#define uitracksettingdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		January 2012
 RCS:		$Id: uitracksettingdlg.h,v 1.1 2012/02/14 23:20:31 cvsyuancheng Exp $
________________________________________________________________________


-*/

#include "uidialog.h"

template<class T> class Array3D;
class uiGenInput;
class uiIOObjSel;
class uiSeisSel;
class uiSeisSubSel;


mClass uiTrackSettingDlg : public uiDialog
{
public:
    			uiTrackSettingDlg(uiParent*);

    bool		acceptOK(CallBacker*);

protected:

    bool		readInput(Array3D<float>&);
    void		modeChangeCB(CallBacker*);
    void		inpSel(CallBacker*);
    bool		processHoughLineExtraction();
    bool		processSeedsFloodFill();

    uiGenInput*		modefld_;
    uiSeisSel*		inputselfld_;
    uiSeisSubSel*	subselfld_;
    uiGenInput*		thresholdfld_;
    uiGenInput*		aboveisovaluefld_;
   
    uiGenInput*		hltoplistfld_;
    uiGenInput*		hlanglergfld_;

    uiIOObjSel*		seedselfld_;
    uiIOObjSel*		outputfltfld_;
    uiIOObjSel*		outputbodyfld_;
};

#endif
