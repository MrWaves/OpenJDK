#
# Copyright (c) 1996, 2007, Oracle and/or its affiliates. All rights reserved.
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
# Makefile for character converters.
#

BUILDDIR = ../..

PACKAGE = sun.nio
PRODUCT = sun

# This re-directs all the class files to a separate location
CLASSDESTDIR = $(TEMPDIR)/classes

OTHER_JAVACFLAGS += -Xlint:serial -Werror
include $(BUILDDIR)/common/Defs.gmk

#
# Files
#
include FILES_java.gmk
AUTO_FILES_JAVA_DIRS = sun/nio/cs/ext

# For Cygwin, command line arguments that are paths must be converted to
# windows style paths. These paths cannot be used as targets, however, because
# the ":" in them  will interfere with GNU Make rules, generating "multiple
# target pattern" errors.

# this define is for the rule:
CHARSETS_JAR = $(LIBDIR)/charsets.jar

#
# Rules
#
include $(BUILDDIR)/common/Classes.gmk

build: $(CHARSETS_JAR)

#
# Extra rules to build character converters.

SERVICE_DESCRIPTION = java.nio.charset.spi.CharsetProvider
SERVICE_DESCRIPTION_PATH = META-INF/services/$(SERVICE_DESCRIPTION)

$(CLASSDESTDIR)/$(SERVICE_DESCRIPTION_PATH): \
  $(SHARE_SRC)/classes/sun/nio/cs/ext/$(SERVICE_DESCRIPTION_PATH)
	$(install-file)

$(CHARSETS_JAR): $(FILES_class) $(CLASSDESTDIR)/$(SERVICE_DESCRIPTION_PATH)
	$(BOOT_JAR_CMD) cf $(CHARSETS_JAR) \
	      -C $(CLASSDESTDIR) sun \
	      -C $(CLASSDESTDIR) $(SERVICE_DESCRIPTION_PATH)  \
	      $(JAR_JFLAGS)
	@$(java-vm-cleanup)

clean::
	$(RM) -r $(CLASSDESTDIR)
	$(RM) $(CHARSETS_JAR)
