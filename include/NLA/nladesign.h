#ifndef nladesign_h
#define nladesign_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2001
 RCS:		$Id: nladesign.h,v 1.3 2003-11-07 12:21:52 bert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"


/*!\brief Simple description of NLA design, viewed from user's perspective.

Note: Currently NN only.
If hiddensz == 0, it will be set to nrinputs / 3, with a minimum of 3.
If nr of outputs == 0, unsupervised network will be assumed. That means the
actual nr of output nodes is 2 (segment and match).

*/

class NLADesign
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
				deepCopy( inputs, sd.inputs );
				deepCopy( outputs, sd.outputs );
				hiddensz = sd.hiddensz;
				doclass = sd.doclass;
			    }
			    return *this;
			}

    inline void		clear()
			{
			    deepErase(inputs); deepErase(outputs);
			    hiddensz = 0; doclass = false;
			}
    inline bool		isSupervised() const
			{ return outputs.size(); }

    BufferStringSet	inputs;
    BufferStringSet	outputs;
    int				hiddensz;
    bool			doclass;

};


#endif
