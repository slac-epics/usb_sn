TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build an ioc application
# Use PROD_IOC_RTEMS for RTEMS only
# Use PROD_IOC_Linux for Linux only
LIBRARY_IOC_Linux = usbsn

# usbsn.dbd will be installed
DBD += usbsn.dbd

# Add any header files here which should be installed
#INC	+=	file.h

# Add any html documentation here
# HTMLS	+=	index.html
# HTMLS +=	ipimb.html
HTMLS_DIR	= .

# Add any source files here
# Note: No lib will be built if no sources are listed here
LIBSRCS += usbcheck.c

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

