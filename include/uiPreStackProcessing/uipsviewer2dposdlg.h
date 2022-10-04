#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "multiid.h"
#include "bufstringset.h"
#include "datapack.h"
#include "rowcol.h"
#include "uistring.h"
#include "uiprestackprocessingmod.h"
#include "uislicesel.h"

class uiListBox;
class uiTable;
class uiPushButton;
class uiToolButton;

namespace PreStackView
{

mStruct(uiPreStackProcessing) GatherInfo
{
			GatherInfo();
			~GatherInfo();

    bool		isstored_		= true;
    bool		isselected_		= false;
    MultiID		mid_;
    DataPackID		vddpid_;
    DataPackID		wvadpid_;
    BufferString	gathernm_;
    BinID		bid_			= BinID::udf();

bool operator==( const GatherInfo& info ) const
{
    return isstored_==info.isstored_ && bid_==info.bid_ &&
	   (isstored_ ? mid_==info.mid_
	   	      : (gathernm_==info.gathernm_));
}

};



mExpClass(uiPreStackProcessing) uiGatherPosSliceSel : public uiSliceSel
{ mODTextTranslationClass(uiGatherPosSliceSel);
public:
				uiGatherPosSliceSel(uiParent*,uiSliceSel::Type,
						    const BufferStringSet&,
						    bool issynthetic=false);
				~uiGatherPosSliceSel();

    const TrcKeyZSampling&	cubeSampling();

    void		setTrcKeyZSampling(const TrcKeyZSampling&) override;
    void		setStep(int);
    int			step() const;

    void		enableZDisplay(bool);
    void		getSelGatherInfos(TypeSet<GatherInfo>&);
    void		setSelGatherInfos(const TypeSet<GatherInfo>&);

protected:
    uiLabeledSpinBox* 		stepfld_;
    uiPushButton*		updbut_;
    uiTable*			posseltbl_;
    BufferStringSet		gathernms_;
    bool			issynthetic_;
    TypeSet<GatherInfo>		gatherinfos_;
    TypeSet<int>		dispgatheridxs_;
    TypeSet<RowCol>		disptblposs_;

    void			resetDispGatherInfos();
    void			reDoTable();

    void			posChged(CallBacker*);
    void			applyPushed(CallBacker*);
    void			gatherChecked(CallBacker*);
    void			gatherPosChanged(CallBacker*);
    void			updatePosTable(CallBacker*);
    void			cellSelectedCB(CallBacker*);
};


mExpClass(uiPreStackProcessing) uiViewer2DPosDlg : public uiDialog
{ mODTextTranslationClass(uiViewer2DPosDlg);
public:
				uiViewer2DPosDlg(uiParent*,bool is2d,
						const TrcKeyZSampling&,
						const BufferStringSet&,
						bool issynthetic);
				~uiViewer2DPosDlg();

    void			setTrcKeyZSampling(const TrcKeyZSampling&);
    void			getTrcKeyZSampling(TrcKeyZSampling&);

    void                        enableZDisplay(bool yn)
				    { sliceselfld_->enableZDisplay(yn); }
    void			getSelGatherInfos(TypeSet<GatherInfo>& infos)
				{ sliceselfld_->getSelGatherInfos(infos); }
    void			setSelGatherInfos(const TypeSet<GatherInfo>& gi)
				{ sliceselfld_->setSelGatherInfos(gi); }

    Notifier<uiViewer2DPosDlg> okpushed_;

protected:

    uiGatherPosSliceSel*	sliceselfld_;
    bool			is2d_;
    bool			acceptOK(CallBacker*) override;
};


mExpClass(uiPreStackProcessing) uiViewer2DSelDataDlg : public uiDialog
{ mODTextTranslationClass(uiViewer2DSelDataDlg);
public:
			    uiViewer2DSelDataDlg(uiParent*,
				    const BufferStringSet&,BufferStringSet&);
			    ~uiViewer2DSelDataDlg();

protected:

    uiListBox*			allgatherfld_;
    uiListBox*			selgatherfld_;
    uiToolButton*		toselect_;
    uiToolButton*		fromselect_;

    BufferStringSet&		selgathers_;

    void 			selButPush(CallBacker*);
    bool			acceptOK(CallBacker*) override;
};

} // namespace PreStackView
