#ifndef uiwvltseissel_h
#define uiwvltseissel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseiswvltsel.h,v 1.5 2011/09/13 15:09:24 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "bufstringset.h"
class MultiID;
class Wavelet;
class uiComboBox;


/*!\brief 'Immediate' Wavelet selector, with 'Manage' button */

mClass uiSeisWaveletSel : public uiGroup
{
public:

			uiSeisWaveletSel(uiParent*,
					 const char* seltxt="Wavelet");
			~uiSeisWaveletSel();
    void		rebuildList();

    const char*		getName() const;
    const MultiID&	getID() const;
    Wavelet*		getWavelet() const;
    void		setInput(const char*);
    void		setInput(const MultiID&);

    Notifier<uiSeisWaveletSel> newSelection;

protected:

    uiComboBox*		nmfld_;
    BufferStringSet	nms_;
    ObjectSet<MultiID>	ids_;

    void		initFlds(CallBacker*);
    void		startMan(CallBacker*);
    void		selChg(CallBacker*);

};


#endif
