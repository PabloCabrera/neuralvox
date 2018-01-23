#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
reload (sys)
sys.setdefaultencoding('utf-8')

import codecs

file = codecs.open ("training.txt", "r", encoding="utf8")
for line in file:
	word = line.replace('\n', '')
	print 'espeak -v es -x -w "wav/%s" "%s" > "transcription/%s"' % (word, word, word)

