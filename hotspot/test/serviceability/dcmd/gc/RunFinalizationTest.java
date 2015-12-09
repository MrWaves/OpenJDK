/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

import java.util.ArrayList;
import java.util.List;

import jdk.test.lib.OutputAnalyzer;
import jdk.test.lib.ProcessTools;

/*
 * @test
 * @summary Test of diagnostic command GC.run_finalization
 * @library /testlibrary
 * @modules java.base/sun.misc
 *          java.compiler
 *          java.management
 *          jdk.jvmstat/sun.jvmstat.monitor
 * @build jdk.test.lib.*
 * @build jdk.test.lib.dcmd.*
 * @build RunFinalizationTest FinalizationRunner
 * @run main RunFinalizationTest
 */
public class RunFinalizationTest {
    private final static String TEST_APP_NAME = "FinalizationRunner";

    public static void main(String ... args) throws Exception {
        List<String> javaArgs = new ArrayList<>();
        javaArgs.add("-cp");
        javaArgs.add(System.getProperty("test.class.path"));
        javaArgs.add(TEST_APP_NAME);
        ProcessBuilder testAppPb = ProcessTools.createJavaProcessBuilder(javaArgs.toArray(new String[javaArgs.size()]));

        OutputAnalyzer out = ProcessTools.executeProcess(testAppPb);
        out.stderrShouldNotMatch("^" + FinalizationRunner.FAILED + ".*")
           .stdoutShouldMatch("^" + FinalizationRunner.PASSED + ".*");
    }
}