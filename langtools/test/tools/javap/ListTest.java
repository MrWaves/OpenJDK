/*
 * Copyright (c) 2008, Oracle and/or its affiliates. All rights reserved.
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

import java.io.*;
import java.util.*;
import javax.tools.*;

/*
 * @test
 * @bug 6439940
 * @summary Cleanup javap implementation
 * @run main/othervm ListTest
 */
public class ListTest {
    public static void main(String[] args) throws Exception {
        new ListTest().run();
    }

    ListTest() {
        String v = System.getProperty("view.cmd");
        //      v = "/opt/teamware/7.7/bin/filemerge -r";
        if (v != null) {
            viewResults = true;
            viewCmd = Arrays.asList(v.split(" +"));
        }
    }

    void run() throws Exception {
        StandardLocation[] locs = new StandardLocation[] {
            StandardLocation.PLATFORM_CLASS_PATH,
            StandardLocation.CLASS_PATH,
        };

        int count = 0;
        int pass = 0;
        for (StandardLocation loc: locs) {
            for (String testClassName: list(loc)) {
                count++;
                if (test(testClassName))
                    pass++;
            }
        }

        if (pass < count)
            throw new Error(pass + "/" + count + " test cases passed");
    }

    Iterable<String> list(StandardLocation loc) throws IOException {
        JavaCompiler compiler = ToolProvider.getSystemJavaCompiler();
        StandardJavaFileManager sfm = compiler.getStandardFileManager(null, null, null);
        Set<JavaFileObject.Kind> kinds = Collections.singleton(JavaFileObject.Kind.CLASS);

        List<String> list = new ArrayList<String>();
        for (JavaFileObject fo: sfm.list(loc, testPackage, kinds, true)) {
            //System.err.println(com.sun.tools.javac.util.Old199.getPath(fo));
            list.add(sfm.inferBinaryName(loc, fo));
        }
        return list;
    }

    boolean test(String testClassName) throws Exception {
        String[] args = new String[options.size() + 1];
        options.toArray(args);
        args[args.length - 1] = testClassName;
        String oldOut = runOldJavap(args);
        String newOut = runNewJavap(args);
        boolean ok = oldOut.equals(newOut);
        System.err.println((ok ? "pass" : "FAIL") + ": " + testClassName);
        if (!ok && viewResults)
            view(oldOut, newOut);
        return ok;
    }

    String runOldJavap(String[] args) {
        //System.err.println("OLD: " + Arrays.asList(args));
        PrintStream oldOut = System.out;
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        System.setOut(new PrintStream(out));
        try {
            sun.tools.javap.Main.entry(args);
        } finally {
            System.setOut(oldOut);
        }
        return out.toString();
    }

    String runNewJavap(String[] args) {
        String[] nArgs = new String[args.length + 2];
        nArgs[0] = "-XDcompat";
        nArgs[1] = "-XDignore.symbol.file";
        System.arraycopy(args, 0, nArgs, 2, args.length);
        //System.err.println("NEW: " + Arrays.asList(nArgs));
        StringWriter out = new StringWriter();
        com.sun.tools.javap.Main.run(nArgs, new PrintWriter(out, true));
        return out.toString();
    }

    File write(String text, String suffix) throws IOException {
        File f = File.createTempFile("ListTest", suffix);
        FileWriter out = new FileWriter(f);
        out.write(text);
        out.close();
        return f;
    }

    void view(String oldOut, String newOut) throws Exception {
        File oldFile = write(oldOut, "old");
        File newFile = write(newOut, "new");
        List<String> cmd = new ArrayList<String>();
        cmd.addAll(viewCmd);
        cmd.add(oldFile.getPath());
        cmd.add(newFile.getPath());
        Process p = new ProcessBuilder(cmd).redirectErrorStream(true).start();
        p.getOutputStream().close();
        String line;
        BufferedReader in = new BufferedReader(new InputStreamReader(p.getInputStream()));
        while ((line = in.readLine()) != null)
            System.err.println(line);
        in.close();
        p.waitFor();
    }

    String testPackage = "java.lang";
    List<String> options = Arrays.asList("-v");
    boolean viewResults;
    List<String> viewCmd;
}
