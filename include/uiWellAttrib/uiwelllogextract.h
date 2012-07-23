#ifndef uiwelllogextract_h
#define uiwelllogextract_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          July 2012
 RCS:           $Id: uiwelllogextract.h,v 1.1 2012-07-23 09:30:01 cvssatyaki Exp $
________________________________________________________________________

-*/


#include "uigroup.h"
class IOObj;
class uiListBox;
class uiGenInput;
class DataPointSet;
class DataPointSetDisplayMgr;
class BufferStringSet;
class uiDataPointSet;
class uiMultiWellLogSel;
class uiPosFilterSetSel;
namespace Attrib { class DescSet; }


mClass uiWellLogExtractGrp : public uiGroup
{
public:
					uiWellLogExtractGrp(uiParent*,
						const Attrib::DescSet* ads=0,
						bool singlelog=false);
					~uiWellLogExtractGrp();

    void				setDescSet(const Attrib::DescSet*);
    void				getWellNames(BufferStringSet&);

    bool				extractDPS();
    const DataPointSet*			getDPS() const;

protected:

    const Attrib::DescSet* ads_;
    ObjectSet<IOObj>	wellobjs_;

    uiListBox*		attrsfld_;
    uiGenInput*		radiusfld_;
    uiGenInput*		logresamplfld_;
    uiMultiWellLogSel*	welllogselfld_;
    uiPosFilterSetSel*	posfiltfld_;
    DataPointSet*	curdps_;

    void		adsChg();
    bool		extractWellData(const BufferStringSet&,
	    				const BufferStringSet&,
					ObjectSet<DataPointSet>&);
    bool		extractAttribData(DataPointSet&,int);
};


#endif
