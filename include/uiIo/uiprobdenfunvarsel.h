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
class MnemonicSelection;
class UnitOfMeasure;
class uiComboBox;
class uiGenInput;
class uiUnitSel;

mExpClass(uiIo) uiPrDenFunVarSel : public uiGroup
{ mODTextTranslationClass(uiPrDenFunVarSel);
public:

		mClass(uiIo) DataColInfo
		{
		public:
			    DataColInfo(const BufferStringSet& colnames,
					const TypeSet<int>& colids,
					const MnemonicSelection&,
					const ObjectSet<const UnitOfMeasure>&);
			    DataColInfo(const DataColInfo&);
			    ~DataColInfo();

		    DataColInfo& operator =(const DataColInfo&);

		    BufferStringSet		colnms_;
		    TypeSet<int>		colids_;
		    MnemonicSelection&		mns_;
		    ObjectSet<const UnitOfMeasure> uoms_;
		};

				uiPrDenFunVarSel(uiParent*,const DataColInfo&,
						 const uiString& lbl,
						 bool withunitsel);
				~uiPrDenFunVarSel();

    int				selNrBins() const;
    int				selColID() const;
    StepInterval<float>		selColRange() const;
    BufferString		selColName() const;
    const char*			colName(int idx) const;
    int				nrCols() const;
    bool			hasAttrib(const char*) const;
    const UnitOfMeasure*	getUnit() const;

    void			setAttrRange(const StepInterval<float>&);
    void			setColNr(int);
    void			setPrefCol(const char*);

    Notifier<uiPrDenFunVarSel>	attrSelChanged;

private:

    DataColInfo			colinfos_;
    uiComboBox*			attrsel_;
    uiGenInput*			rangesel_;
    uiGenInput*			nrbinsel_;
    uiUnitSel*			unitfld_ = nullptr;

    void			initGrp(CallBacker*);
    void			attrChanged(CallBacker*);
    void			nrBinChanged(CallBacker*);
    void			rangeChanged(CallBacker*);

};
