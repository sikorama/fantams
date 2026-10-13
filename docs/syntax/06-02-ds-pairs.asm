; §6 Emission directives — `ds` takes several count,value pairs on one line:
; three bytes of 1, then three of 2.
        org #100
from:   ds 3,1,3,2
past:
        assert past - from == 6
