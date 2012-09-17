#ifndef uiprobdenfunvarsel_h
#define uiprobdenfunvarsel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          March 2010
________________________________________________________________________

-*/

#include "uigroup.h"

class BufferStringSet;
class uiComboBox;
class uiGenInput;
template < class T > class StepInterval;

mClass uiPrDenFunVarSel : public uiGroup
{
public:

struct DataColInfo
{
			    DataColInfo(const BufferStringSet& colnames,
				    	const TypeSet<int>& colids)
				: colnms_(colnames), colids_(colids) {}

    BufferStringSet			colnms_;
    TypeSet<int>			colids_;
};

				uiPrDenFunVarSel(uiParent*, const DataColInfo&);
    int 			selNrBins() const;
    int 			selColID() const;
    StepInterval<float>		selColRange() const;
    BufferString		selColName() const;

    void			setAttrRange(const StepInterval<float>&);
    void			setColNr(int);

    Notifier<uiPrDenFunVarSel>	attrSelChanged;

protected:

    DataColInfo			colinfos_;
    uiComboBox*			attrsel_;
    uiGenInput* 		rangesel_;
    uiGenInput* 		nrbinsel_;

    void			attrChanged(CallBacker*);
    void			nrBinChanged(CallBacker*);
    void			rangeChanged(CallBacker*);

public:
    const char*			colName(int idx) const;
    int				nrCols() const;
    void			setPrefCol(const char*);
};

#endif
