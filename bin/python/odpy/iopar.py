#
# (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# AUTHOR   : Bert
# DATE     : August 2018
#
# IOPar tools
#

import os

def read_line( words ):
  words = words.split(':')
  cleanwords = [x.strip() for x in words]
  nrwords = len(cleanwords)
  ret = list()
  if nrwords > 0:
    ret.append( cleanwords[0] )
  if nrwords > 1:
    ret.append( ':'.join(cleanwords[1:nrwords]) )
  else:
    ret.append( ':'.join('') )
  return ret

def read_from_iopar( fnm, searchkey ):
  if not os.path.exists(fnm):
    return None

  with open( fnm, 'r' ) as fp:
    for line in fp:
      key,val = read_line(line)
      if key == searchkey:
        return val

  return None
