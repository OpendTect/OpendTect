#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2001
________________________________________________________________________

-*/

#include "nlamod.h"
#include "bufstringset.h"


/*!
\brief Simple description of NLA design, viewed from user's perspective.

  Note: Currently NN only.
  If hiddensz == 0, it will be set to nrinputs / 2, with a minimum of 1.
  If nr of outputs == 0, unsupervised network will be assumed. That means the
  actual nr of output nodes is 2 (segment and match). If classification is true,
  two extra output nodes will be added ('Classification' and 'Confidence').
*/

mExpClass(NLA) NLADesign
{
public:
			NLADesign()	{ clear(); }
			~NLADesign()	{ clear(); }
			NLADesign( const NLADesign& sd )
					{ *this = sd; }
    NLADesign&		operator =( const NLADesign& sd )
			{
			    if ( this != &sd )
			    {
				inputs = sd.inputs;
				outputs = sd.outputs;
				hiddensz = sd.hiddensz;
				classification = sd.classification;
			    }
			    return *this;
			}

    inline void		clear()
			{
			    inputs.setEmpty(); outputs.setEmpty();
			    hiddensz = 0; classification = false;
			}
    inline bool		isSupervised() const
			{ return !outputs.isEmpty(); }

    static inline int	finalNrHiddenNodes( int usrsz, int nrinp )
			{
			    if ( usrsz < 1 || mIsUdf(usrsz) )
			    {
				usrsz = nrinp / 2;
				if ( usrsz < 1 )
				    usrsz = 1;
			    }
			    return usrsz;
			}

    BufferStringSet	inputs;
    BufferStringSet	outputs;
    int			hiddensz;
    bool		classification;

};
