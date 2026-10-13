; §2 Numbers — '$' alone is the current address; '$FF' stays a number.
        org #4000
here:
        assert $ == here
        assert $FF == 255
        assert $ + $FF == #40FF
