/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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
 *
 */


#ifndef SHARE_VM_CLASSFILE_SYSTEMDICTIONARYSHARED_HPP
#define SHARE_VM_CLASSFILE_SYSTEMDICTIONARYSHARED_HPP

#include "classfile/sharedClassUtil.hpp"
#include "classfile/systemDictionary.hpp"

class SystemDictionaryShared: public SystemDictionary {
public:
  static void initialize(TRAPS) {}
  static instanceKlassHandle find_or_load_shared_class(Symbol* class_name,
                                                       Handle class_loader,
                                                       TRAPS) {
    return instanceKlassHandle();
  }
  static void roots_oops_do(OopClosure* blk) {}
  static void oops_do(OopClosure* f) {}
  static bool is_sharing_possible(ClassLoaderData* loader_data) {
    oop class_loader = loader_data->class_loader();
    return (class_loader == NULL);
  }
  static bool is_shared_class_visible_for_classloader(
                                      Symbol* class_name,
                                      instanceKlassHandle ik,
                                      Handle class_loader,
                                      TRAPS) {
    debug_only( {
      int index = ik->shared_classpath_index();
      SharedClassPathEntry* ent =
            (SharedClassPathEntry*)FileMapInfo::shared_classpath(index);
      assert(ent->is_jrt(), "must from the bootmodules.jimage");
      assert(class_loader.is_null(), "Unsupported classloader");
    } );
    return true;
  }
};

#endif // SHARE_VM_CLASSFILE_SYSTEMDICTIONARYSHARED_HPP