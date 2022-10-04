#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "callback.h"
#include "integerid.h"
#include "uistring.h"

class DataPointSet;
class uiODApplMgr;
class uiParent;

/*!\brief Does visualization-related work for uiODApplMgr */

mExpClass(uiODMain) uiODApplMgrAttrVisHandler : public CallBacker
{ mODTextTranslationClass(uiODApplMgrAttrVisHandler);
    friend class	uiODApplMgr;

			uiODApplMgrAttrVisHandler(uiODApplMgr&,uiParent*);
			~uiODApplMgrAttrVisHandler();

    void		survChg(bool);
    void		saveNLA(CallBacker*);
    bool		editNLA(bool);
    bool		uvqNLA(bool);
    void		createHorOutput(int tp,bool);
    void		createVol(bool is2d,bool multiattrib);
    void		doXPlot();
    void		crossPlot();
    void		setZStretch();
    bool		selectAttrib(VisID,int);
    void		setHistogram(VisID,int);
    void		colMapperChg();
    void		setRandomPosData(VisID visid,int attrib,
					const DataPointSet&);
    void		pageUpDownPressed(bool);
    void		updateColorTable(VisID,int);
    void		colSeqChg();
    NotifierAccess*	colorTableSeqChange();
    void		useDefColTab(VisID,int);
    void		saveDefColTab(VisID,int);

    uiODApplMgr&	am_;
    uiParent*		par_;
};
