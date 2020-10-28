#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Bert
# DATE     : August 2018
#
# IOPar tools
#

import odpy.common


def read_line( fp, bin ):
  if bin:
    words = fp.readline().decode("utf8").split(":")
  else:
    words = fp.readline().split(":")
  cleanwords = [x.strip() for x in words]
  nrwords = len(cleanwords)
  ret = list()
  if nrwords > 0:
    ret.append( cleanwords[0] )
  if nrwords > 1:
    ret.append( ':'.join(cleanwords[1:nrwords]) )
  return ret
