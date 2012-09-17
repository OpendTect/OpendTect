#ifndef uiseispreloadmgr_h
#define uiseispreloadmgr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
 RCS:           $Id: uisynthtorealscale.h,v 1.4 2011/09/13 14:14:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

#include "horsampling.h"
#include "multiid.h"

class BinIDValueSet;
class SeisTrcBuf;
class TaskRunner;
class uiSeisSel;
class uiIOObjSel;
class uiGenInput;
class uiStratSeisEvent;
class uiSynthToRealScaleStatsDisp;
template <class T> class ODPolygon;

namespace EM { class Horizon3D; class Horizon; }


/*!\brief To determine scaling of synthetics using real data.
 
  Note: the input trc buf *must* have ref times in the trc.info().pick's.

 */

mClass uiSynthToRealScale : public uiDialog
{ 	
public:

			uiSynthToRealScale(uiParent*,bool is2d,SeisTrcBuf&,
					   const MultiID& wvltid,
					   const char* reflvlnm);

    const MultiID&	inpWvltID() const	{ return inpwvltid_; }
    const MultiID&	selWvltID() const	{ return outwvltid_; }

protected:

    bool		is2d_;
    SeisTrcBuf&		synth_;
    MultiID		inpwvltid_;
    MultiID		outwvltid_;

    uiSeisSel*		seisfld_;
    uiIOObjSel*		horfld_;
    uiIOObjSel*		polyfld_;
    uiIOObjSel*		wvltfld_;
    uiStratSeisEvent*	evfld_;
    uiGenInput*		finalscalefld_;
    uiSynthToRealScaleStatsDisp*	synthstatsfld_;
    uiSynthToRealScaleStatsDisp*	realstatsfld_;

    void		initWin(CallBacker*);
    void		setScaleFld(CallBacker*);
    void		goPush( CallBacker* )
    			{ updSynthStats(); updRealStats(); }
    bool		acceptOK(CallBacker*);

    void		updSynthStats();
    void		updRealStats();
    bool		getBinIDs(BinIDValueSet&);

    struct DataSelection
    {
			    DataSelection()
				: polygon_(0), polyhs_(false), horizon_(0)  {}
			    ~DataSelection();

	void		    setHorizon(EM::Horizon*);
	ODPolygon<float>*   polygon_;
	HorSampling	    polyhs_;
	EM::Horizon*	    horizon_;
    };

    bool		getPolygon(DataSelection&) const;
    bool		getHorizon(DataSelection&,TaskRunner*) const;
    bool		getBinIDs(BinIDValueSet&,const DataSelection&) const;
};


#endif
