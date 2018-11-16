#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Arnaud
# DATE     : November 2018
#
# ranges tools
#

import odpy.common
import numpy as np

def get_range_steps( samp ):
  rg = range( samp[0], samp[1]+samp[2], samp[2] )
  return np.linspace( rg.start, rg.stop, len(rg), dtype=np.int32 )

def getSampling( rg ):
  return {
    'lines': get_range_steps( rg['Inline'] ),
    'traces': get_range_steps( rg['Crossline'] ),
    'zsamp': get_range_steps( rg['Z'] )
  }
