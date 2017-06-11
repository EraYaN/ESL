include arm.base.setup.mk

#   ----------------------------------------------------------------------------
#   Variant configuration
#   ----------------------------------------------------------------------------
CFLAGS += -mfpu=neon         # Enable compile NEON support (Required for NEON intrinsics)
DEFS += -DNEON               # Enable NEON intrinsics
#DEFS += -DFIXEDPOINT         # Enable Fixed-point math
DEFS += -DDSP                # Enable DSP (mutually exclusive with DSP_ONLY)
#DEFS += -DDSP_ONLY           # Enable DSP and disbale CPU support (mutually exclusive with DSP)

#DEFS += -DTIMING             # Enable fine-grained TIMING for pdf, calweight and track.
#DEFS += -DTIMING2            # Enable fine-grained TIMING for the balancing function.
#DEFS += -DDEBUGPRINT         # Enable DEBUG messages.
#DEFS += -WRITE_DYN_RANGE     # Enable writing all intermediate values toa a dynrange.csv file. (very slow, does not work with every variant)

include arm.base.targets.mk