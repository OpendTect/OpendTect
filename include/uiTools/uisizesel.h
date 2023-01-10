#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

#include <array>

class uiSpinBox;

mExpClass(uiTools) uiSizeSel : public uiGroup
{
public:
			uiSizeSel(uiParent*,const uiString& lbl,int maxnrdim);
			~uiSizeSel();

    int			maxNrDim() const;
    void		setNrDim(int);
    int			currentNrDim() const;
    void		setSymmetric(bool yn);

    void		setImageSize(int dim,int sz);
    int			getImageSize(int dim) const;

    void		setSizeRange(int dim,const StepInterval<int>&);

    void		setPrefix(int dim,const uiString&);
    void		setDefaultPrefixes();
			//!<1dim: Z; 2dim: Nr,Z; 3 dim: Inl,Crl,Z

// For convenience
    void		setImageSize(std::array<int,2>);
    void		setImageSize(std::array<int,3>);
    std::array<int,2>	getImageSize2D() const;
    std::array<int,3>	getImageSize3D() const;

    Notifier<uiSizeSel> valueChanging;

protected:
    ObjectSet<uiSpinBox>	sizeflds_;

    void		valueChangingCB(CallBacker*);
};
