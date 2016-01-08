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

/*
 * @test
 * @bug 8136421
 * @requires (os.simpleArch == "x64" | os.simpleArch == "sparcv9") & os.arch != "aarch64"
 * @library / /testlibrary /test/lib/
 * @ignore 8143238
 * @compile ../common/CompilerToVMHelper.java
 * @build compiler.jvmci.compilerToVM.ShouldDebugNonSafepointsTest
 * @run main ClassFileInstaller
 *     jdk.vm.ci.hotspot.CompilerToVMHelper
 * @run main/othervm
 *     -XX:+UnlockExperimentalVMOptions -XX:+EnableJVMCI -Xbootclasspath/a:.
 *     -XX:+UnlockDiagnosticVMOptions
 *     -XX:+DebugNonSafepoints
 *     -Dcompiler.jvmci.compilerToVM.ShouldDebugNonSafepointsTest.expected=true
 *     compiler.jvmci.compilerToVM.ShouldDebugNonSafepointsTest
 * @run main/othervm
 *     -XX:+UnlockExperimentalVMOptions -XX:+EnableJVMCI -Xbootclasspath/a:.
 *     -XX:+UnlockDiagnosticVMOptions
 *     -XX:-DebugNonSafepoints
 *     -Dcompiler.jvmci.compilerToVM.ShouldDebugNonSafepointsTest.expected=false
 *     compiler.jvmci.compilerToVM.ShouldDebugNonSafepointsTest
 */

package compiler.jvmci.compilerToVM;

import jdk.vm.ci.hotspot.CompilerToVMHelper;
import jdk.test.lib.Asserts;

public class ShouldDebugNonSafepointsTest {
    private static final boolean EXPECTED = Boolean.getBoolean("compiler"
            + ".jvmci.compilerToVM.ShouldDebugNonSafepointsTest.expected");

    public static void main(String args[]) {
        new ShouldDebugNonSafepointsTest().runTest();
    }

    private void runTest() {
        Asserts.assertEQ(CompilerToVMHelper.shouldDebugNonSafepoints(),
                EXPECTED, "Unexpected shouldDebugnonSafepoints value");
    }
}