/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

import java.nio.file.attribute.FileTime;
import jdk.internal.jimage.ImageReader.Node;

/**
 * File attributes implementation for jrt image file system.
 */
final class JrtFileAttributes extends AbstractJrtFileAttributes {

    private final Node node;

    JrtFileAttributes(Node node) {
        this.node = node;
    }

    ///////// basic attributes ///////////
    @Override
    public FileTime creationTime() {
        return node.creationTime();
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
        return !isDirectory();
    }

    @Override
    public FileTime lastAccessTime() {
        return node.lastAccessTime();
    }

    @Override
    public FileTime lastModifiedTime() {
        return node.lastModifiedTime();
    }

    @Override
    public long size() {
        return node.size();
    }

    @Override
    public boolean isSymbolicLink() {
        return node.isLink();
    }

    @Override
    public Object fileKey() {
        return node.resolveLink(true);
    }

    ///////// jrt entry attributes ///////////
    @Override
    public long compressedSize() {
        return node.compressedSize();
    }

    @Override
    public String extension() {
        return node.extension();
    }
}