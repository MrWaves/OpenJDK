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
package jdk.internal.jrtfs;

import java.io.IOException;
import java.nio.file.attribute.BasicFileAttributes;
import java.nio.file.attribute.FileTime;
import jdk.internal.jrtfs.JrtExplodedFileSystem.Node;

/**
 * jrt file system attributes implementation on top of 'exploded file system'
 * Node.
 */
final class JrtExplodedFileAttributes extends AbstractJrtFileAttributes {

    private final Node node;
    private final BasicFileAttributes attrs;

    JrtExplodedFileAttributes(Node node) throws IOException {
        this.node = node;
        this.attrs = node.getBasicAttrs();
    }

    @Override
    public FileTime creationTime() {
        return attrs.creationTime();
    }

    @Override
    public boolean isDirectory() {
        return node.isDirectory();
    }

    @Override
    public boolean isOther() {
        return false;
    }

    @Override
    public boolean isRegularFile() {
        return node.isFile();
    }

    @Override
    public FileTime lastAccessTime() {
        return attrs.lastAccessTime();
    }

    @Override
    public FileTime lastModifiedTime() {
        return attrs.lastModifiedTime();
    }

    @Override
    public long size() {
        return isRegularFile() ? attrs.size() : 0L;
    }

    @Override
    public boolean isSymbolicLink() {
        return node.isLink();
    }

    @Override
    public Object fileKey() {
        return node.resolveLink(true);
    }

    @Override
    public long compressedSize() {
        return 0L;
    }

    @Override
    public String extension() {
        return node.getExtension();
    }
}