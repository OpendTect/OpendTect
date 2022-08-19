#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "nlamod.h"
#include "bufstringset.h"
#include "typeset.h"


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
				inputs_ = sd.inputs_;
				outputs_ = sd.outputs_;
				hiddensz_ = sd.hiddensz_;
				classification_ = sd.classification_;
			    }
			    return *this;
			}

    inline void		clear()
			{
			    inputs_.setEmpty(); outputs_.setEmpty();
			    hiddensz_.setEmpty(); hiddensz_ += 0;
			    classification_ = false;
			}

    inline bool		isSupervised() const
			{ return outputs_.size(); }

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

    BufferStringSet	inputs_;
    BufferStringSet	outputs_;
    TypeSet<int>	hiddensz_;
    bool		classification_;

};
