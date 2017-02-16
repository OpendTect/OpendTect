#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uicolseqsel.h"
#include "coltabmappersetup.h"
#include "datadistribution.h"
class uiColTabSelTool;
class uiManipMapperSetup;
class uiEdMapperSetupDlg;


/*!\brief The color table selection tool on the OD main window. */

mGlobal(uiTools) uiColTabSelTool& uiCOLTAB();


/*!\brief Full Color Table = Sequence + Mapper Setup selection tool. */

mExpClass(uiTools) uiColTabSelTool : public uiColSeqSelTool
{ mODTextTranslationClass(uiColTabSelTool);
public:

    typedef ColTab::MapperSetup		MapperSetup;
    typedef DataDistribution<float>	DistribType;
    typedef DataDistributionIter<float>	DistribIterType;

				~uiColTabSelTool();

				// access current
    RefMan<MapperSetup>		mapperSetup()		{ return mappersetup_; }
    ConstRefMan<MapperSetup>	mapperSetup() const	{ return mappersetup_; }
    RefMan<DistribType>		distribution()		{ return distrib_; }
    ConstRefMan<DistribType>	distribution() const	{ return distrib_; }

				// make selector use another
    void			useMapperSetup(const MapperSetup&);
    void			useDistribution(const DistribType&);

    void			setRange( Interval<float> rg )
				{ mappersetup_->setRange( rg ); }

    // Convenience. If you need to know what happened, ask the objs
    Notifier<uiColTabSelTool>	mapperSetupChanged;
    Notifier<uiColTabSelTool>	distributionChanged;

    Notifier<uiColTabSelTool>	mapperMenuReq;
				//!< CallBacker* is the uiMenu about to pop up

    virtual void		addObjectsToToolBar(uiToolBar&);

protected:

			uiColTabSelTool();

    RefMan<MapperSetup>	mappersetup_;
    RefMan<DistribType>	distrib_;

    uiManipMapperSetup*	manip_;

    void		initialise(OD::Orientation);

    void		mapSetupChgCB(CallBacker*);
    void		distribChgCB(CallBacker*);
    void		newManDlgCB(CallBacker*);

    friend class	uiManipMapperSetup;
    friend class	uiEdMapperSetupDlg;

};


/* uiGroup for color table selection */

mExpClass(uiTools) uiColTabSel : public uiGroup
			       , public uiColTabSelTool
{
public:

			uiColTabSel(uiParent*,OD::Orientation,
				uiString lbl=uiString::emptyString());

			mImpluiColSeqSelGroup();

};


/* uiToolBar for color table selection */

mExpClass(uiTools) uiColTabToolBar : public uiToolBar
{ mODTextTranslationClass(uiColTabToolBar);
public:

			uiColTabToolBar(uiParent*);

			mImpluiColSeqSelToolBar(uiColTabSelTool);

};
