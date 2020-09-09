set(VSSDKROOT $ENV{VSSDK100Install}/VisualStudioIntegration)
set(VSSDKINCLUDE ${VSSDKROOT}/Common/Inc)
set(VSSDKLIB ${VSSDKROOT}/Common/Lib)
set(VSSDKBIN ${VSSDKROOT}/Tools/Bin)

include_directories(${VSSDKINCLUDE})
include_directories(${CMAKE_CURRENT_LIST_DIR}/../compat/vssdk)
