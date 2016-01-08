/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
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
package jdk.internal.misc;

import java.io.IOException;
import java.io.InputStream;
import java.lang.module.ModuleReference;
import java.lang.reflect.Module;
import java.net.URL;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.stream.Stream;

import jdk.internal.module.ServicesCatalog;

/**
 * Find resources and packages in modules defined to the boot class loader or
 * resources and packages on the "boot class path" specified via -Xbootclasspath/a.
 */

public class BootLoader {
    private BootLoader() { }

    // The unnamed module for the boot loader
    private static final Module UNNAMED_MODULE;
    static {
        UNNAMED_MODULE
            = SharedSecrets.getJavaLangReflectModuleAccess().defineUnnamedModule(null);
        setBootLoaderUnnamedModule0(UNNAMED_MODULE);
    }

    // ServiceCatalog for the boot class loader
    private static final ServicesCatalog SERVICES_CATALOG = new ServicesCatalog();

    /**
     * Returns the unnamed module for the boot loader.
     */
    public static Module getUnnamedModule() {
        return UNNAMED_MODULE;
    }

    /**
     * Returns the ServiceCatalog for modules defined to the boot class loader.
     */
    public static ServicesCatalog getServicesCatalog() {
        return SERVICES_CATALOG;
    }

    /**
     * Register a module with this class loader so that its classes (and
     * resources) become visible via this class loader.
     */
    public static void loadModule(ModuleReference mref) {
        ClassLoaders.bootLoader().loadModule(mref);
    }

    /**
     * Loads the Class object with the given name defined to the boot loader.
     */
    public static Class<?> loadClassOrNull(String name) {
        return ClassLoaders.bootLoader().loadClassOrNull(name);
    }

    /**
     * Returns a URL to a resource in a named module defined to the boot loader.
     */
    public static URL findResource(String mn, String name) throws IOException {
        return ClassLoaders.bootLoader().findResource(mn, name);
    }

    /**
     * Returns an input stream to a resource in a named module defined to the
     * boot loader.
     */
    public static InputStream findResourceAsStream(String mn, String name)
        throws IOException
    {
        return ClassLoaders.bootLoader().findResourceAsStream(mn, name);
    }

    /**
     * Returns the URL to the given resource if the resource can be located
     * on the boot class path. This method does not locate a resource in any
     * of the named modules defined to the boot loader.
     */
    public static URL findResource(String name) {
        return ClassLoaders.bootLoader().findResource(name);
    }

    /**
     * Returns an Iterator to iterate over the resources of the given name
     * on the boot class path. This method does not locate resources in any
     * of the named modules defined to the boot loader.
     */
    public static Enumeration<URL> findResources(String name) throws IOException {
        return ClassLoaders.bootLoader().findResources(name);
    }

    /**
     * Define a package for the given class to the boot loader, if not already
     * defined.
     */
    public static Package definePackage(Class<?> c) {
        return ClassLoaders.bootLoader().definePackage(c);
    }

    /**
     * Returns the Package of the given name defined to the boot loader or null
     * if the package has not been defined.
     */
    public static Package getDefinedPackage(String pn) {
        String found = getSystemPackageLocation(pn.replace('.', '/'));
        if (found == null) {
            return null;
        }
        return ClassLoaders.bootLoader().definePackage(pn);
    }

    /**
     * Returns a stream of the packages defined to the boot loader.
     */
    public static Stream<Package> packages() {
        return Arrays.stream(getSystemPackageNames())
            .map(name -> {
                String pn = name.replace('/', '.');
                return ClassLoaders.bootLoader().definePackage(pn);
            });
    }

    private static native String getSystemPackageLocation(String name);
    private static native String[] getSystemPackageNames();
    private static native void setBootLoaderUnnamedModule0(Module module);
}