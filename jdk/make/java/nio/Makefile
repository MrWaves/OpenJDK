#
# Copyright (c) 2000, 2007, Oracle and/or its affiliates. All rights reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
#
# This code is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 only, as
# published by the Free Software Foundation.  Oracle designates this
# particular file as subject to the "Classpath" exception as provided
# by Oracle in the LICENSE file that accompanied this code.
#
# This code is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# version 2 for more details (a copy is included in the LICENSE file that
# accompanied this code).
#
# You should have received a copy of the GNU General Public License version
# 2 along with this work; if not, write to the Free Software Foundation,
# Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
# or visit www.oracle.com if you need additional information or have any
# questions.
#

#
# Makefile for java.nio
#

BUILDDIR = ../..
PACKAGE = java.nio
LIBRARY = nio
PRODUCT = java
OTHER_JAVACFLAGS += -Xmaxwarns 1000 -Xlint:serial -Werror
include $(BUILDDIR)/common/Defs.gmk

NIO_SRC = $(SHARE_SRC)/classes/java/nio
NIO_GEN = $(GENSRCDIR)/java/nio

SNIO_SRC = $(SHARE_SRC)/classes/sun/nio
SNIO_GEN = $(GENSRCDIR)/sun/nio

#
# Files to compile
#

include FILES_java.gmk
include FILES_c.gmk
include Exportedfiles.gmk

ifeq ($(PLATFORM), solaris)
FILES_java += \
        sun/nio/ch/AbstractPollSelectorImpl.java \
	sun/nio/ch/DevPollArrayWrapper.java \
	sun/nio/ch/DevPollSelectorImpl.java \
        sun/nio/ch/DevPollSelectorProvider.java \
	sun/nio/ch/InheritedChannel.java \
        sun/nio/ch/PollSelectorProvider.java \
        sun/nio/ch/PollSelectorImpl.java

FILES_c += \
        DevPollArrayWrapper.c \
	InheritedChannel.c \
        PollArrayWrapper.c \
	NativeThread.c

FILES_export += \
	sun/nio/ch/DevPollArrayWrapper.java \
	sun/nio/ch/InheritedChannel.java \
	sun/nio/ch/NativeThread.java
endif # PLATFORM = solaris

ifeq ($(PLATFORM), windows)
FILES_java += \
        sun/nio/ch/WindowsSelectorImpl.java \
        sun/nio/ch/WindowsSelectorProvider.java

FILES_c += \
        WindowsSelectorImpl.c

FILES_export += \
	sun/nio/ch/WindowsSelectorImpl.java
endif # PLATFORM = windows

ifeq ($(PLATFORM), linux)
FILES_java += \
        sun/nio/ch/AbstractPollSelectorImpl.java \
	sun/nio/ch/EPollArrayWrapper.java \
	sun/nio/ch/EPollSelectorProvider.java \
	sun/nio/ch/EPollSelectorImpl.java \
	sun/nio/ch/InheritedChannel.java \
        sun/nio/ch/PollSelectorProvider.java \
        sun/nio/ch/PollSelectorImpl.java

FILES_c += \
	EPollArrayWrapper.c \
        PollArrayWrapper.c \
	InheritedChannel.c \
	NativeThread.c

FILES_export += \
        sun/nio/ch/EPollArrayWrapper.java \
	sun/nio/ch/InheritedChannel.java \
	sun/nio/ch/NativeThread.java
endif # PLATFORM = linux

# Find platform-specific C source files
#
vpath %.c $(PLATFORM_SRC)/native/sun/nio/ch
vpath %.c $(SHARE_SRC)/native/sun/nio/ch

#
# Various variables
#

JAVALIB = 		# Don't self-link

#
# Access to io_util.c and net_util.c
#

OTHER_INCLUDES += \
  -I$(SHARE_SRC)/native/sun/nio/ch \
  -I$(SHARE_SRC)/native/java/io \
  -I$(SHARE_SRC)/native/java/net \
  -I$(PLATFORM_SRC)/native/java/net \
  -I$(CLASSHDRDIR)/../../../java.lang/java/CClassHeaders

ifeq ($(PLATFORM),windows)
  OTHER_LDLIBS += $(JVMLIB) ws2_32.lib \
     -libpath:$(LIBDIR) java.lib \
     $(OBJDIR)/../../../../sun/java.net/net/$(OBJDIRNAME)/net.lib \
     $(OBJDIR)/../../../java.lang/java/$(OBJDIRNAME)/io_util.obj \
     $(OBJDIR)/../../../java.lang/java/$(OBJDIRNAME)/FileDescriptor_md.obj
endif
ifeq ($(PLATFORM), linux)
OTHER_LDLIBS += -L$(LIBDIR)/$(LIBARCH) -ljava -lnet -lpthread -ldl
endif
ifeq ($(PLATFORM), solaris)
OTHER_LDLIBS += $(JVMLIB) $(LIBSOCKET) -lposix4 -ldl \
		-L$(LIBDIR)/$(LIBARCH) -ljava -lnet
endif # PLATFORM

#
# Rules
#

build: sources

clean clobber::
	$(RM) -r $(NIO_GEN) $(SNIO_GEN)
	$(RM) -r $(CLASSDESTDIR)/java/nio
	$(RM) -r $(CLASSDESTDIR)/sun/nio

ifeq ($(PLATFORM), solaris)
FILES_m = mapfile-solaris
endif
ifeq ($(PLATFORM), linux)
FILES_m = mapfile-linux
endif
include $(BUILDDIR)/common/Mapfile-vers.gmk
include $(BUILDDIR)/common/Library.gmk

#
# Generate source files
#

SPP_JARFILE = $(BUILDTOOLJARDIR)/spp.jar
SPP_CMD = $(BOOT_JAVA_CMD) -jar $(SPP_JARFILE)

FILES_genout = $(FILES_gen:%.java=$(GENSRCDIR)/%.java)

BUF_SRC=$(NIO_SRC)
CH_SRC=$(NIO_SRC)/channels
CS_SRC=$(NIO_SRC)/charset
SCH_SRC=$(SNIO_SRC)/ch
SCS_SRC=$(SNIO_SRC)/cs

# Template files
HEAP_X_BUF_TEMPLATE=$(BUF_SRC)/Heap-X-Buffer.java.template
BYTE_X_BUF_TEMPLATE=$(BUF_SRC)/ByteBufferAs-X-Buffer.java.template
X_BUF_TEMPLATE=$(BUF_SRC)/X-Buffer.java.template
X_BUF_BIN_TEMPLATE=$(BUF_SRC)/X-Buffer-bin.java.template
DIRECT_X_BUF_TEMPLATE=$(BUF_SRC)/Direct-X-Buffer.java.template
DIRECT_X_BUF_BIN_TEMPLATE=$(BUF_SRC)/Direct-X-Buffer-bin.java.template
CHARSET_X_CODER_TEMPLATE=$(CS_SRC)/Charset-X-Coder.java.template

BUF_GEN=$(NIO_GEN)
CH_GEN=$(NIO_GEN)/channels
CS_GEN=$(NIO_GEN)/charset
SCH_GEN=$(SNIO_GEN)/ch
SCS_GEN=$(SNIO_GEN)/cs

sources: $(SPP_JARFILE) $(FILES_genout)

#
# Generated buffer classes
#

GEN_BUFFER_SH = genBuffer.sh

GEN_BUFFER_CMD = SPP="$(SPP_CMD)" NAWK="$(NAWK)" SED="$(SED)" \
                 SH="$(SH)" $(SH) $(GEN_BUFFER_SH)

# Public abstract buffer classes
#
$(BUF_GEN)/ByteBuffer.java: $(X_BUF_TEMPLATE) \
			    $(X_BUF_BIN_TEMPLATE) \
			    $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=byte BIN=1 SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/CharBuffer.java: $(X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ShortBuffer.java: $(X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/IntBuffer.java: $(X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/LongBuffer.java: $(X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/FloatBuffer.java: $(X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DoubleBuffer.java: $(X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@

# Buffers whose contents are heap-allocated
#
$(BUF_GEN)/HeapByteBuffer.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=byte SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapByteBuffer%.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=byte RW=$* SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapCharBuffer.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapCharBuffer%.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char RW=$* SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapShortBuffer.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapShortBuffer%.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short RW=$* SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapIntBuffer.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapIntBuffer%.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int RW=$* SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapLongBuffer.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapLongBuffer%.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long RW=$* SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapFloatBuffer.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapFloatBuffer%.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float RW=$* SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapDoubleBuffer.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/HeapDoubleBuffer%.java: $(HEAP_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double RW=$* SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@

# Direct byte buffer
#
$(BUF_GEN)/DirectByteBuffer.java: $(DIRECT_X_BUF_TEMPLATE) \
				  $(DIRECT_X_BUF_TEMPLATE) \
				  $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=byte BIN=1 SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectByteBuffer%.java: $(DIRECT_X_BUF_TEMPLATE) \
				   $(DIRECT_X_BUF_TEMPLATE) \
				   $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=byte RW=$* BIN=1 SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@

# Unswapped views of direct byte buffers
#
$(BUF_GEN)/DirectCharBufferU.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectCharBuffer%U.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char RW=$* BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectShortBufferU.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectShortBuffer%U.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short RW=$* BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectIntBufferU.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectIntBuffer%U.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int RW=$* BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectLongBufferU.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectLongBuffer%U.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long RW=$* BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectFloatBufferU.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectFloatBuffer%U.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float RW=$* BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectDoubleBufferU.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectDoubleBuffer%U.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double RW=$* BO=U SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@

# Swapped views of direct byte buffers
#
$(BUF_GEN)/DirectCharBufferS.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectCharBuffer%S.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char RW=$* BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectShortBufferS.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectShortBuffer%S.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short RW=$* BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectIntBufferS.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectIntBuffer%S.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int RW=$* BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectLongBufferS.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectLongBuffer%S.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long RW=$* BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectFloatBufferS.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectFloatBuffer%S.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float RW=$* BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectDoubleBufferS.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/DirectDoubleBuffer%S.java: $(DIRECT_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double RW=$* BO=S SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@

# Big-endian views of byte buffers
#
$(BUF_GEN)/ByteBufferAsCharBufferB.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsCharBuffer%B.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char RW=$* BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsShortBufferB.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsShortBuffer%B.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short RW=$* BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsIntBufferB.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsIntBuffer%B.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int RW=$* BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsLongBufferB.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsLongBuffer%B.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long RW=$* BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsFloatBufferB.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsFloatBuffer%B.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float RW=$* BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsDoubleBufferB.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsDoubleBuffer%B.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double RW=$* BO=B SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@

# Little-endian views of byte buffers
#
$(BUF_GEN)/ByteBufferAsCharBufferL.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsCharBuffer%L.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=char RW=$* BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsShortBufferL.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsShortBuffer%L.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=short RW=$* BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsIntBufferL.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsIntBuffer%L.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=int RW=$* BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsLongBufferL.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsLongBuffer%L.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=long RW=$* BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsFloatBufferL.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsFloatBuffer%L.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=float RW=$* BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsDoubleBufferL.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@
$(BUF_GEN)/ByteBufferAsDoubleBuffer%L.java: $(BYTE_X_BUF_TEMPLATE) $(GEN_BUFFER_SH)
	$(prep-target)
	@$(RM) $@.temp
	TYPE=double RW=$* BO=L SRC=$< DST=$@.temp $(GEN_BUFFER_CMD)
	$(MV) $@.temp $@

#
# Generated coder classes
#

GEN_CODER_SH = genCoder.sh

GEN_CODER_CMD = SPP="$(SPP_CMD)" SED="$(SED)" NAWK="$(NAWK)" SH="$(SH)" $(SH) $(GEN_CODER_SH)

$(CS_GEN)/CharsetDecoder.java: $(CHARSET_X_CODER_TEMPLATE) $(GEN_CODER_SH)
	$(prep-target)
	@$(RM) $@.temp
	$(GEN_CODER_CMD) decoder $< $@.temp
	$(MV) $@.temp $@

$(CS_GEN)/CharsetEncoder.java: $(CHARSET_X_CODER_TEMPLATE) $(GEN_CODER_SH)
	$(prep-target)
	@$(RM) $@.temp
	$(GEN_CODER_CMD) encoder $< $@.temp
	$(MV) $@.temp $@

#
# Generated exception classes
#

GEN_EX_SH = genExceptions.sh

GEN_EX_CMD = NAWK="$(NAWK)" SH="$(SH)" $(SH) $(GEN_EX_SH)

$(CH_GEN)/%Exception.java: genExceptions.sh $(CH_SRC)/exceptions
	$(prep-target)
	@$(RM) $@.temp
	$(GEN_EX_CMD) $(CH_SRC)/exceptions $(CH_GEN)

$(CS_GEN)/%Exception.java: genExceptions.sh $(CS_SRC)/exceptions
	$(prep-target)
	@$(RM) $@.temp
	$(GEN_EX_CMD) $(CS_SRC)/exceptions $(CS_GEN)

$(BUF_GEN)/%Exception.java: genExceptions.sh $(BUF_SRC)/exceptions
	$(prep-target)
	@$(RM) $@.temp
	$(GEN_EX_CMD) $(BUF_SRC)/exceptions $(BUF_GEN)

$(SCH_GEN)/%Exception.java: genExceptions.sh $(SCH_SRC)/exceptions
	$(prep-target)
	@$(RM) $@.temp
	$(GEN_EX_CMD) $(SCH_SRC)/exceptions $(SCH_GEN)


#
# Generated charset-provider classes
#

HASHER_JARFILE = $(BUILDTOOLJARDIR)/hasher.jar

$(SCS_GEN)/StandardCharsets.java: genCharsetProvider.sh \
    $(HASHER_JARFILE) $(SCS_SRC)/standard-charsets
	$(prep-target)
	@$(RM) $@.temp
	NAWK="$(NAWK)" TEMPDIR="$(TEMPDIR)" \
	  HASHER="$(BOOT_JAVA_CMD) -jar $(HASHER_JARFILE)" \
	  SH="$(SH)" $(SH) -e genCharsetProvider.sh $(SCS_SRC)/standard-charsets $(SCS_GEN)

.PHONY: sources
