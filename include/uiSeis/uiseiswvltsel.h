#ifndef uiseissel_h
#define uiseissel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseiswvltsel.h,v 1.3 2010-12-07 16:15:06 cvsbert Exp $
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
    void		fillBox();

};


#endif
