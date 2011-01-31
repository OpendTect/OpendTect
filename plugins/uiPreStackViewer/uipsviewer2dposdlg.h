#ifndef uipsviewer2dposdlg_h
#define uipsviewer2dposdlg_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2011
 RCS:           $Id: uipsviewer2dposdlg.h,v 1.1 2011-01-31 13:03:50 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uislicesel.h"

class uiCheckBox;

namespace PreStackView
{

mClass uiGatherPosSliceSel : public uiSliceSel
{
public:
				uiGatherPosSliceSel(uiParent*,uiSliceSel::Type);

    const CubeSampling&		cubeSampling(); 
    void			setNrViewers(int);
    int				nrViewers() const;
    int 			step() const;			

    uiLabeledSpinBox* 		stepfld_;
    uiLabeledSpinBox*		nrviewersfld_;
    uiCheckBox*			dynamicrgbox_;

    void			enableDynamicRange(bool);
    void			enableZDisplay(bool);

    void			dynamicRangeChged(CallBacker*);
    void			parsChged(CallBacker*);
    void			applyPushed(CallBacker*);
};

mClass uiViewer2DPosDlg : public uiDialog
{
public:
				uiViewer2DPosDlg(uiParent*,const CubeSampling&);

    void 			setCubeSampling( const CubeSampling& cs)
					{ sliceselfld_->setCubeSampling(cs); }
    void 			getCubeSampling( CubeSampling& cs)
					{ cs =sliceselfld_->getCubeSampling(); }
    void                        setNrViewers(int nrv)
				    { sliceselfld_->setNrViewers(nrv); }
    int				nrViewers() const
				    { return sliceselfld_->nrViewers(); }
    void                        enableDynamicRange(bool yn)
				    { sliceselfld_->enableDynamicRange(yn); }
    void                        enableZDisplay(bool yn)
				    { sliceselfld_->enableZDisplay(yn); }


protected:

    uiGatherPosSliceSel*	sliceselfld_;
};


}; //namespace

#endif
