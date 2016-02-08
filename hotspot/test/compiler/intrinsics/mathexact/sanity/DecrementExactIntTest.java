/*
 * Copyright (c) 2013, 2015, Oracle and/or its affiliates. All rights reserved.
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
 * @library /testlibrary /test/lib /compiler/whitebox / /compiler/testlibrary
 * @modules java.base/sun.misc
 *          java.management
 * @build DecrementExactIntTest
 * @run main ClassFileInstaller sun.hotspot.WhiteBox
 *                              sun.hotspot.WhiteBox$WhiteBoxPermission
 * @run main/othervm -Xbootclasspath/a:. -XX:+UnlockDiagnosticVMOptions
 *                   -XX:+IgnoreUnrecognizedVMOptions -XX:+WhiteBoxAPI -XX:+LogCompilation
 *                   -XX:CompileCommand=compileonly,MathIntrinsic*::execMathMethod
 *                   -XX:LogFile=hs_neg.log -XX:-UseMathExactIntrinsics DecrementExactIntTest
 * @run main/othervm -Xbootclasspath/a:. -XX:+UnlockDiagnosticVMOptions
 *                   -XX:+IgnoreUnrecognizedVMOptions -XX:+WhiteBoxAPI -XX:+LogCompilation
 *                   -XX:CompileCommand=compileonly,MathIntrinsic*::execMathMethod
 *                   -XX:LogFile=hs.log -XX:+UseMathExactIntrinsics DecrementExactIntTest
 * @run main intrinsics.Verifier hs_neg.log hs.log
 */

public class DecrementExactIntTest {

    public static void main(String[] args) throws Exception {
        new IntrinsicBase.IntTest(MathIntrinsic.IntIntrinsic.Decrement).test();
    }
}