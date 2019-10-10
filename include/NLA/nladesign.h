#ifndef nladesign_h
#define nladesign_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		June 2001
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
				deepCopy( inputs_, sd.inputs_ );
				deepCopy( outputs_, sd.outputs_ );
				hiddensz_ = sd.hiddensz_;
				classification_ = sd.classification_;
			    }
			    return *this;
			}

    inline void		clear()
			{
			    deepErase(inputs_); deepErase(outputs_);
			    hiddensz_.erase(); hiddensz_ += 0;
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


#endif
