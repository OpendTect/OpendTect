#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"

class BufferStringSet;
class uiComboBox;
class uiGenInput;

mExpClass(uiIo) uiPrDenFunVarSel : public uiGroup
{ mODTextTranslationClass(uiPrDenFunVarSel);
public:

		struct DataColInfo
		{
		    DataColInfo(const BufferStringSet& colnames,
				const TypeSet<int>& colids);
		    ~DataColInfo();

		    BufferStringSet			colnms_;
		    TypeSet<int>			colids_;
		};

				uiPrDenFunVarSel(uiParent*, const DataColInfo&);
				uiPrDenFunVarSel(uiParent*,const DataColInfo&,
						 const uiString& lbl);
				~uiPrDenFunVarSel();

    int 			selNrBins() const;
    int 			selColID() const;
    StepInterval<float>		selColRange() const;
    BufferString		selColName() const;
    const char*			colName(int idx) const;
    int				nrCols() const;

    void			setAttrRange(const StepInterval<float>&);
    void			setColNr(int);
    void			setPrefCol(const char*);

    Notifier<uiPrDenFunVarSel>	attrSelChanged;

protected:

    DataColInfo			colinfos_;
    uiComboBox*			attrsel_;
    uiGenInput* 		rangesel_;
    uiGenInput* 		nrbinsel_;

    void			attrChanged(CallBacker*);
    void			nrBinChanged(CallBacker*);
    void			rangeChanged(CallBacker*);

    void			createGUI(uiObject* attachobj);
};
